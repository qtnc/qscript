#include "Fiber.hpp"
#include "VM.hpp"
#include "BoundFunction.hpp"
#include "ExtraAlgorithms.hpp"
#include "OpCodeInfo.hpp"
#include "Fiber_inlines.hpp"
#include<string>
using namespace std;

static void instantiate (QFiber& f) {
int ctorSymbol = f.vm.findMethodSymbol("constructor");
QClass& cls = f.getObject<QClass>(0);
if (ctorSymbol>=cls.methods.size() || cls.methods[ctorSymbol].isNull()) {
f.runtimeError(("This class isn't instantiable"));
return;
}
QObject* instance = cls.instantiate();
f.setObject(0, instance);
f.callSymbol(ctorSymbol, f.getArgCount());
f.returnValue(instance);
}

void QVM::bindGlobal (const string& name, const QV& value) {
int symbol = findGlobalSymbol(name, true);
insert_n(globalVariables, 1+symbol-globalVariables.size(), QV());
globalVariables.at(symbol) = value;
}

QClass* QVM::createNewClass (const string& name, vector<QV>& parents, int nStaticFields, int nFields, bool foreign) {
string metaName = name + ("MetaClass");
QClass* parent = parents[0].asObject<QClass>();
QClass* meta = QClass::create(*this, classClass, classClass, metaName, 0, nStaticFields);
QClass* cls = foreign?
new QForeignClass(*this, meta, parent, name, nFields):
QClass::create(*this, meta, parent, name, nStaticFields, nFields+std::max(0, parent->nFields));
for (auto& p: parents) cls->mergeMixinMethods( p.asObject<QClass>() );
meta->bind("()", instantiate);
return cls;
}


void QFiber::pushNewClass (int nParents, int nStaticFields, int nFields) {
string name = at(-nParents -1).asString();
vector<QV> parents(stack.end() -nParents, stack.end());
stack.erase(stack.end() -nParents -1, stack.end());
QClass* parent = parents[0].asObject<QClass>();
if (parent->nFields<0) {
runtimeError("Unable to inherit from built-in class %s", parent->name);
return;
}
if (dynamic_cast<QForeignClass*>(parent)) {
runtimeError("Unable to inherit from foreign class %s", parent->name);
return;
}
QClass* cls = vm.createNewClass(name, parents, nStaticFields, nFields, false);
push(cls);
}

void QFiber::pushNewForeignClass (const std::string& name, size_t id, int nUserBytes, int nParents) {
if (nParents<=0) {
push(vm.objectClass);
nParents = 1;
}
vector<QV> parents(stack.end() -nParents, stack.end());
QForeignClass* cls = static_cast<QForeignClass*>(vm.createNewClass(name, parents, 0, nUserBytes, true));
cls->id = id;
stack.erase(stack.end() -nParents, stack.end());
push(cls);
vm.foreignClassIds[id] = cls;
}

void QFiber::loadGlobal (const string& name) {
int symbol = vm.findGlobalSymbol(name, false);
if (symbol<0) pushNull();
else push(vm.globalVariables.at(symbol));
}

void QFiber::storeGlobal (const string& name) {
vm.bindGlobal(name, top());
}

void QFiber::storeMethod (const string& name) {
storeMethod(vm.findMethodSymbol(name));
}

void QFiber::storeStaticMethod (const string& name) {
storeStaticMethod(vm.findMethodSymbol(name));
}

void QFiber::storeDestructor ( void(*destr)(void*) ) {
stack.back().asObject<QForeignClass>() ->destructor = destr;
}

void adjustFieldOffset (QFunction& func, int offset) {
for (const char *bc = func.bytecode.data(), *end = func.bytecode.data()+func.bytecode.length(); bc<end; ) {
uint8_t op = *bc++;
switch(op) {
case OP_LOAD_FIELD:
case OP_STORE_FIELD:
*reinterpret_cast<uint_field_index_t*>(const_cast<char*>(bc)) += offset;
break;
}
bc += OPCODE_INFO[op].nArgs;
}
}

void* QFiber::pushNewUserPointer (size_t id) {
QForeignClass& cls = *vm.foreignClassIds[id];
QForeignInstance* instance = static_cast<QForeignInstance*>(cls.instantiate());
push(instance);
return &instance->userData[0];
}

void* QFiber::setNewUserPointer (int idx, size_t id) {
QForeignClass& cls = *vm.foreignClassIds[id];
QForeignInstance* instance = static_cast<QForeignInstance*>(cls.instantiate());
at(idx) = instance;
return &instance->userData[0];
}

void QFiber::import (const std::string& baseFile, const std::string& requestedFile) {
LOCK_SCOPE(vm.globalMutex)
string finalFile = vm.pathResolver(baseFile, requestedFile);
auto it = vm.imports.find(finalFile);
if (it!=vm.imports.end()) push(it->second);
else {
vm.imports[finalFile] = true;
loadFile(finalFile);
call(0);
vm.imports[finalFile] = at(-1);
}}
