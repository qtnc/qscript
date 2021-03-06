#include "SwanLib.hpp"
#include "../vm/Fiber_inlines.hpp"
#include "../vm/BoundFunction.hpp"
#include "../../include/cpprintf.hpp"
using namespace std;


static void fiberInstantiate (QFiber& f) {
if (f.getArgCount()==1) f.returnValue(QV(&f, QV_TAG_FIBER));
else if (f.at(1).isClosure()) {
QClosure& closure = f.getObject<QClosure>(1);
QFiber* fb = f.vm.construct<QFiber>(f.vm, closure);
f.returnValue(QV(fb, QV_TAG_FIBER));
}
else f.returnValue(QV::UNDEFINED);
}

static void fiberNext (QFiber& f) {
f.callFiber(f.getObject<QFiber>(0), f.getArgCount() -1); 
f.returnValue(f.at(1)); 
}

static void objectInstantiate (QFiber& f) {
int ctorSymbol = f.vm.findMethodSymbol("constructor");
QClass& cls = f.getObject<QClass>(0);
if (ctorSymbol>=cls.methods.size() || cls.methods[ctorSymbol].isNullOrUndefined()) {
error<invalid_argument>("%s can't be instantiated, it has no method 'constructor'", cls.name);
return;
}
QObject* instance = cls.instantiate();
f.setObject(0, instance);
f.pushCppCallFrame();
f.callSymbol(ctorSymbol, f.getArgCount());
f.popCppCallFrame();
f.returnValue(instance);
}

void objectHashCode (QFiber& f) {
uint32_t h = FNV_OFFSET, *u = reinterpret_cast<uint32_t*>(&(f.at(0).i));
h ^= u[0] ^u[1];
f.returnValue(static_cast<double>(h));
}

static void objectToString (QFiber& f) {
const QClass& cls = f.at(0).getClass(f.vm);
f.returnValue(format("%s@%#0$16llX", cls.name, f.at(0).i) );
}

static void instanceofOperator (QFiber& f) {
QClass& cls1 = f.getObject<QClass>(0);
QClass& cls2 = f.at(1).getClass(f.vm);
if (cls2.isSubclassOf(f.vm.classClass)) {
QClass& cls3 = f.getObject<QClass>(1);
f.returnValue(cls3.isSubclassOf(&cls1) || cls2.isSubclassOf(&cls1));
}
else f.returnValue(cls2.isSubclassOf(&cls1));
}

static void objectEquals (QFiber& f) {
f.returnValue(f.at(0).i == f.at(1).i);
}

static void objectNotEquals (QFiber& f) {
// Call == so that users don't have do define != explicitly to implement full equality comparisons
QV x1 = f.at(0), x2 = f.at(1);
f.pushCppCallFrame();
f.push(x1);
f.push(x2);
f.callSymbol(f.vm.findMethodSymbol("=="), 2);
bool re = f.at(-1).asBool();
f.pop();
f.popCppCallFrame();
f.returnValue(!re);
}

static void objectLess (QFiber& f) {
// Implement < in termes of compare so that users only have to implement three way comparison
QV x1 = f.at(0), x2 = f.at(1);
f.pushCppCallFrame();
f.push(x1);
f.push(x2);
f.callSymbol(f.vm.findMethodSymbol("compare"), 2);
double re = f.at(-1).asNum();
f.pop();
f.popCppCallFrame();
f.returnValue(re<0);
}


static void objectGreater (QFiber& f) {
// Implement >, >= and <= in termes of < so that users only have to implement < for full comparison
QV x1 = f.at(0), x2 = f.at(1);
f.pushCppCallFrame();
f.push(x2);
f.push(x1);
f.callSymbol(f.vm.findMethodSymbol("<"), 2);
bool re = f.at(-1).asBool();
f.pop();
f.popCppCallFrame();
f.returnValue(re);
}

static void objectLessEquals (QFiber& f) {
// Implement >, >= and <= in termes of < so that users only have to implement < for full comparison
QV x1 = f.at(0), x2 = f.at(1);
f.pushCppCallFrame();
f.push(x2);
f.push(x1);
f.callSymbol(f.vm.findMethodSymbol("<"), 2);
bool re = f.at(-1).asBool();
f.pop();
f.popCppCallFrame();
f.returnValue(!re);
}

static void objectGreaterEquals (QFiber& f) {
// Implement >, >= and <= in termes of < so that users only have to implement < for full comparison
QV x1 = f.at(0), x2 = f.at(1);
f.pushCppCallFrame();
f.push(x1);
f.push(x2);
f.callSymbol(f.vm.findMethodSymbol("<"), 2);
bool re = f.at(-1).asBool();
f.pop();
f.popCppCallFrame();
f.returnValue(!re);
}

static void functionInstantiate (QFiber& f) {
f.returnValue(QV::UNDEFINED);
if (f.isString(1)) {
QV adctx = f.getArgCount()>=3? f.at(2) : QV::UNDEFINED;
f.loadString(f.getString(1), "<eval>", "<eval>", adctx);
f.returnValue(f.at(-1));
}
else if (f.isNum(1)) {
int index = f.getNum(1) -1;
if (index<0 && -index<=f.callFrames.size()) {
auto cf = f.callFrames.end() +index;
if (cf->closure) f.returnValue(QV(cf->closure, QV_TAG_CLOSURE));
}}
}

static void functionBind (QFiber& f) {
f.returnValue(QV(BoundFunction::create(f.vm, f.at(0), f.getArgCount() -1, &f.at(1)), QV_TAG_BOUND_FUNCTION));
}

static string getFuncName (QV f, QVM& vm) {
if (f.isClosure()) return f.asObject<QClosure>()->func.name;
else if (f.isNormalFunction()) return f.asObject<QFunction>()->name;
else if (f.isGenericSymbolFunction()) return "::" + vm.methodSymbols[f.asInt<uint_method_symbol_t>()];
else if (f.isNativeFunction() || f.isStdFunction()) return "<native>";
else if (f.isFiber()) return getFuncName(QV(f.asObject<QFiber>() ->callFrames[0] .closure, QV_TAG_CLOSURE), vm) + ":<fiber>";
else if (f.isBoundFunction()) {
BoundFunction& bf = *f.asObject<BoundFunction>();
return format("bound(%d):%s", bf.count, getFuncName(bf.method, vm));
}
else return "<unknown>";
}

void QVM::initBaseTypes () {
objectClass
->copyParentMethods()
BIND_N(constructor)
BIND_L(class, { f.returnValue(&f.at(0).getClass(f.vm)); })
BIND_F(toString, objectToString)
BIND_F(is, objectEquals)
BIND_F(==, objectEquals)
BIND_F(!=, objectNotEquals)
->bind(findMethodSymbol("compare"), QV::UNDEFINED)
BIND_F(<, objectLess)
BIND_F(>, objectGreater)
BIND_F(<=, objectLessEquals)
BIND_F(>=, objectGreaterEquals)
BIND_L(!, { f.returnValue(QV(false)); })
BIND_L(?, { f.returnValue(QV(true)); })
;

classClass
->copyParentMethods()
BIND_F( (), objectInstantiate)
BIND_F(is, instanceofOperator)
BIND_L(toString, { f.returnValue(QV(f.vm, f.getObject<QClass>(0) .name)); })
BIND_F(hashCode, objectHashCode)
BIND_L(name, { f.returnValue(QV(f.vm, f.getObject<QClass>(0) .name)); })
;

functionClass
->copyParentMethods()
//##BIND_L( (), {  f.callFunc(f.at(0), f.getArgCount() -1);  f.returnValue(f.at(1));  })
BIND_F(hashCode, objectHashCode)
BIND_F(bind, functionBind)
BIND_L(name, { f.returnValue(getFuncName(f.at(0), f.vm)); })
;

boolClass
->copyParentMethods()
BIND_L(!, { f.returnValue(!f.getBool(0)); })
BIND_N(?)
BIND_L(toString, { f.returnValue(QV(f.vm, f.getBool(0)? "true" : "false")); })
BIND_F(hashCode, objectHashCode)
;

nullClass
->copyParentMethods()
BIND_L(!, { f.returnValue(QV::TRUE); })
BIND_L(?, { f.returnValue(QV::FALSE); })
BIND_L(toString, { f.returnValue(QV(f.vm, "null", 4));  })
BIND_L(==, { f.returnValue(f.isNullOrUndefined(1)); })
BIND_F(hashCode, objectHashCode)
;

undefinedClass
->copyParentMethods()
BIND_L(!, { f.returnValue(QV::TRUE); })
BIND_L(?, { f.returnValue(QV::FALSE); })
BIND_L(toString, { f.returnValue(QV(f.vm, "undefined", 9));  })
BIND_L(==, { f.returnValue(f.isNullOrUndefined(1)); })
BIND_F(hashCode, objectHashCode)
;

iterableClass
->copyParentMethods()
BIND_N(iterator)
;

fiberClass
->copyParentMethods()
BIND_F( (), fiberNext)
BIND_F(next, fiberNext)
BIND_N(iterator)
;

classClass->type
->copyParentMethods()
;

objectClass->type
->copyParentMethods()
;

boolClass ->type
->copyParentMethods()
BIND_L( (), { f.returnValue(!f.at(1).isFalsy()); })
;

functionClass ->type
->copyParentMethods()
BIND_F( (), functionInstantiate)
;

fiberClass ->type
->copyParentMethods()
BIND_F( (), fiberInstantiate)
;

iterableClass ->type
->copyParentMethods()
;

iteratorClass ->type
->copyParentMethods()
;
}
