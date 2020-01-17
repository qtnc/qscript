#include "Expression.hpp"
#include "Statement.hpp"
#include "TypeInfo.hpp"
#include "ParserRules.hpp"
#include "Compiler.hpp"
#include "../vm/VM.hpp"
using namespace std;

unordered_map<int,int> BASE_OPTIMIZED_OPS = {
#define OP(N,M) { T_##N, OP_##M }
OP(PLUS, ADD),
OP(MINUS, SUB),
OP(STAR, MUL),
OP(SLASH, DIV),
OP(BACKSLASH, INTDIV),
OP(PERCENT, MOD),
OP(STARSTAR, POW),
OP(LTLT, LSH),
OP(GTGT, RSH),
OP(BAR, BINOR),
OP(AMP, BINAND),
OP(CIRC, BINXOR),
OP(EQEQ, EQ),
OP(EXCLEQ, NEQ),
OP(LT, LT),
OP(GT, GT),
OP(LTE, LTE),
OP(GTE, GTE),
#undef OP
};



static inline bool isUnpack (shared_ptr<Expression> expr) {
return expr->isUnpack();
}

void ConstantExpression::compile (QCompiler& compiler) {
QV& value = token.value;
if (value.isUndefined()) compiler.writeOp(OP_LOAD_UNDEFINED);
else if (value.isNull()) compiler.writeOp(OP_LOAD_NULL);
else if (value.isFalse()) compiler.writeOp(OP_LOAD_FALSE);
else if (value.isTrue()) compiler.writeOp(OP_LOAD_TRUE);
else if (value.isInt8()) compiler.writeOpArg<int8_t>(OP_LOAD_INT8, static_cast<int>(value.d));
else compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(token.value));
}

void DupExpression::compile (QCompiler& compiler) {
compiler.writeOp(OP_DUP);
}

bool LiteralSequenceExpression::isVararg () { 
return any_of(items.begin(), items.end(), ::isUnpack); 
}

void LiteralListExpression::compile (QCompiler& compiler) {
compiler.writeDebugLine(nearestToken());
int listSymbol = compiler.vm.findGlobalSymbol(("List"), LV_EXISTING | LV_FOR_READ);
if (isSingleSequence()) {
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, listSymbol);
items[0]->compile(compiler);
compiler.writeOpCallFunction(1);
} else {
bool vararg = isVararg();
int callSymbol = compiler.vm.findMethodSymbol(("()"));
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, listSymbol);
for (auto item: items) {
compiler.writeDebugLine(item->nearestToken());
item->compile(compiler);
}
int ofSymbol = compiler.vm.findMethodSymbol("of");
if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, ofSymbol);
else compiler.writeOpCallMethod(items.size(), ofSymbol);
}}


void LiteralSetExpression::compile (QCompiler& compiler) {
int setSymbol = compiler.vm.findGlobalSymbol(("Set"), LV_EXISTING | LV_FOR_READ);
compiler.writeDebugLine(nearestToken());
if (isSingleSequence()) {
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, setSymbol);
items[0]->compile(compiler);
compiler.writeOpCallFunction(1);
} else {
bool vararg = isVararg();
int callSymbol = compiler.vm.findMethodSymbol(("()"));
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, setSymbol);
for (auto item: items) {
compiler.writeDebugLine(item->nearestToken());
item->compile(compiler);
}
int ofSymbol = compiler.vm.findMethodSymbol("of");
if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, ofSymbol);
else compiler.writeOpCallMethod(items.size(), ofSymbol);
}}

void LiteralMapExpression::compile (QCompiler& compiler) {
vector<std::shared_ptr<Expression>> unpacks;
int mapSymbol = compiler.vm.findGlobalSymbol(("Map"), LV_EXISTING | LV_FOR_READ);
int subscriptSetterSymbol = compiler.vm.findMethodSymbol(("[]="));
int callSymbol = compiler.vm.findMethodSymbol(("()"));
compiler.writeDebugLine(nearestToken());
if (items.size()==1 && items[0].first->isComprehension()) {
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, mapSymbol);
items[0].first->compile(compiler);
compiler.writeOpCallFunction(1);
return;
}
for (auto it = items.begin(); it!=items.end(); ) {
auto expr = it->first;
if (expr->isUnpack()) {
unpacks.push_back(expr);
it = items.erase(it);
}
else ++it;
}
bool vararg = !unpacks.empty();
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, mapSymbol);
for (auto item: unpacks) {
compiler.writeDebugLine(item->nearestToken());
item->compile(compiler);
}
int ofSymbol = compiler.vm.findMethodSymbol("of");
if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, ofSymbol);
else compiler.writeOp(OP_CALL_FUNCTION_0);
for (auto item: items) {
compiler.writeOp(OP_DUP);
compiler.writeDebugLine(item.first->nearestToken());
item.first->compile(compiler);
compiler.writeDebugLine(item.second->nearestToken());
item.second->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_3, subscriptSetterSymbol);
compiler.writeOp(OP_POP);
}}

void LiteralTupleExpression::compile (QCompiler& compiler) {
int tupleSymbol = compiler.vm.findGlobalSymbol(("Tuple"), LV_EXISTING | LV_FOR_READ);
compiler.writeDebugLine(nearestToken());
if (isSingleSequence()) {
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, tupleSymbol);
items[0]->compile(compiler);
compiler.writeOpCallFunction(1);
} else {
bool vararg = any_of(items.begin(), items.end(), ::isUnpack);
int callSymbol = compiler.vm.findMethodSymbol(("()"));
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, tupleSymbol);
for (auto item: items) {
compiler.writeDebugLine(item->nearestToken());
item->compile(compiler);
}
int ofSymbol = compiler.vm.findMethodSymbol("of");
if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, ofSymbol);
else compiler.writeOpCallMethod(items.size(), ofSymbol);
}}

void LiteralGridExpression::compile (QCompiler& compiler) {
int gridSymbol = compiler.vm.findGlobalSymbol(("Grid"), LV_EXISTING | LV_FOR_READ);
int size = data.size() * data[0].size();
int argLimit = std::numeric_limits<uint_local_index_t>::max() -5;
compiler.writeDebugLine(nearestToken());
if (size>= argLimit) compiler.writeOp(OP_PUSH_VARARG_MARK);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, gridSymbol);
compiler.writeOpArg<uint8_t>(OP_LOAD_INT8, data[0].size());
compiler.writeOpArg<uint8_t>(OP_LOAD_INT8, data.size());
for (auto& row: data) {
for (auto& value: row) {
value->compile(compiler);
}}
if (size>argLimit) compiler.writeOp(OP_CALL_FUNCTION_VARARG);
else compiler.writeOpCallFunction(size+2);
}
void LiteralRegexExpression::compile (QCompiler& compiler) {
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, compiler.findGlobalVariable({ T_NAME, "Regex", 5, QV::UNDEFINED }, LV_EXISTING | LV_FOR_READ));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(QString::create(compiler.parser.vm, pattern), QV_TAG_STRING)));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(QString::create(compiler.parser.vm, options), QV_TAG_STRING)));
compiler.writeOp(OP_CALL_FUNCTION_2);
}


void AnonymousLocalExpression::compile (QCompiler& compiler) { 
compiler.writeOpLoadLocal(token.value.d); 
}

void AnonymousLocalExpression::compileAssignment (QCompiler& compiler, std::shared_ptr<Expression> assignedValue) { 
assignedValue->compile(compiler); 
compiler.writeOpStoreLocal(token.value.d); 
}

void GenericMethodSymbolExpression::compile (QCompiler& compiler) {
int symbol = compiler.parser.vm.findMethodSymbol(std::string(token.start, token.length));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(symbol | QV_TAG_GENERIC_SYMBOL_FUNCTION)));
}

void UnpackExpression::compile (QCompiler& compiler) {
expr->compile(compiler);
compiler.writeOp(OP_UNPACK_SEQUENCE);
}

void TypeHintExpression::compile (QCompiler& compiler)  { 
//todo: actually exploit the type hint
//int pos = compiler.out.tellp();
//compiler.out.seekp(pos);
expr->compile(compiler); 
} 

void TypeHintExpression::compileAssignment (QCompiler& compiler, std::shared_ptr<Expression> assignedValue) {
auto assignable = dynamic_pointer_cast<Assignable>(expr);
if (assignable) assignable->compileAssignment(compiler, assignedValue);
else compiler.compileError(expr->nearestToken(), "Invalid target for assignment");
}

bool TypeHintExpression::isAssignable () {
auto assignable = dynamic_pointer_cast<Assignable>(expr);
return assignable && assignable->isAssignable();
}


bool AbstractCallExpression::isVararg () { 
return  receiver->isUnpack()  || any_of(args.begin(), args.end(), ::isUnpack); 
}

void AbstractCallExpression::compileArgs (QCompiler& compiler) {
for (auto arg: args) arg->compile(compiler);
}

void YieldExpression::compile (QCompiler& compiler) {
if (expr) expr->compile(compiler);
else compiler.writeOp(OP_LOAD_UNDEFINED);
compiler.writeOp(OP_YIELD);
auto method = compiler.getCurMethod();
if (method && expr) method->returnTypeHint = compiler.mergeTypes(method->returnTypeHint, expr->getType(compiler));
}

void ImportExpression::compile (QCompiler& compiler) {
doCompileTimeImport(compiler.parser.vm, compiler.parser.filename, from);
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, compiler.findGlobalVariable({ T_NAME, "import", 6, QV::UNDEFINED }, LV_EXISTING | LV_FOR_READ));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, compiler.findConstant(QV(compiler.parser.vm, compiler.parser.filename)));
from->compile(compiler);
compiler.writeOp(OP_CALL_FUNCTION_2);
}

void ComprehensionExpression::compile (QCompiler  & compiler) {
QCompiler fc(compiler.parser);
fc.parent = &compiler;
rootStatement->optimizeStatement()->compile(fc);
QFunction* func = fc.getFunction(0);
func->name = "<comprehension>";
compiler.result = fc.result;
int funcSlot = compiler.findConstant(QV(func, QV_TAG_NORMAL_FUNCTION));
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, compiler.vm.findGlobalSymbol("Fiber", LV_EXISTING | LV_FOR_READ));
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CLOSURE, funcSlot);
compiler.writeOp(OP_CALL_FUNCTION_1);
}


void NameExpression::compile (QCompiler& compiler) {
if (token.type==T_END) token = compiler.parser.curMethodNameToken;
LocalVariable* lv = nullptr;
int slot = compiler.findLocalVariable(token, LV_EXISTING | LV_FOR_READ, &lv);
if (slot==0 && compiler.getCurClass()) {
compiler.writeOp(OP_LOAD_THIS);
type = lv->type;
return;
}
else if (slot>=0) { 
compiler.writeOpLoadLocal(slot);
type = lv->type;
return;
}
slot = compiler.findUpvalue(token, LV_FOR_READ, &lv);
if (slot>=0) { 
compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, slot);
type = lv->type;
return;
}
slot = compiler.findGlobalVariable(token, LV_EXISTING | LV_FOR_READ, &lv);
if (slot>=0) { 
type = lv->type;
compiler.writeOpArg<uint_global_symbol_t>(OP_LOAD_GLOBAL, slot);
return;
}
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
if (cls) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
if (atLevel<=2) compiler.writeOp(OP_LOAD_THIS);
else compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, compiler.findUpvalue(thisToken, LV_EXISTING | LV_FOR_READ));
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, compiler.vm.findMethodSymbol(string(token.start, token.length)));
type = compiler.resolveCallType(make_shared<NameExpression>(thisToken), token);
return;
}
compiler.compileError(token, ("Undefined variable"));
}

void NameExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
LocalVariable* lv = nullptr;
if (token.type==T_END) token = compiler.parser.curMethodNameToken;
assignedValue->compile(compiler);
type = assignedValue->getType(compiler);
int slot = compiler.findLocalVariable(token, LV_EXISTING | LV_FOR_WRITE, &lv);
if (slot>=0) {
compiler.writeOpStoreLocal(slot);
type = lv->type = compiler.mergeTypes(lv->type, type);
return;
}
else if (slot==LV_ERR_CONST) {
compiler.compileError(token, ("Constant cannot be reassigned"));
return;
}
slot = compiler.findUpvalue(token, LV_FOR_WRITE, &lv);
if (slot>=0) {
compiler.writeOpArg<uint_upvalue_index_t>(OP_STORE_UPVALUE, slot);
type = lv->type = compiler.mergeTypes(lv->type, type);
return;
}
else if (slot==LV_ERR_CONST) {
compiler.compileError(token, ("Constant cannot be reassigned"));
return;
}
slot = compiler.findGlobalVariable(token, LV_EXISTING | LV_FOR_WRITE, &lv);
if (slot>=0) {
compiler.writeOpArg<uint_global_symbol_t>(OP_STORE_GLOBAL, slot);
type = lv->type = compiler.mergeTypes(lv->type, type);
return;
}
else if (slot==LV_ERR_CONST) {
compiler.compileError(token, ("Constant cannot be reassigned"));
return;
}
else if (slot==LV_ERR_ALREADY_EXIST) {
compiler.compileError(token, ("Already existing variable"));
return;
}
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
if (cls) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
if (atLevel<=2) compiler.writeOp(OP_LOAD_THIS);
else compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, compiler.findUpvalue(thisToken, LV_EXISTING | LV_FOR_READ));
char setterName[token.length+2];
memcpy(&setterName[0], token.start, token.length);
setterName[token.length+1] = 0;
setterName[token.length] = '=';
QToken setterNameToken = { T_NAME, setterName, token.length+1, QV::UNDEFINED };
assignedValue->compile(compiler);
//todo: update type of field var
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_2, compiler.vm.findMethodSymbol(setterName));
return;
}
compiler.compileError(token, ("Undefined variable"));
}

void FieldExpression::compile (QCompiler& compiler) {
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
if (!cls) {
compiler.compileError(token, ("Can't use field outside of a class"));
return;
}
if (compiler.getCurMethod()->flags&FD_STATIC) {
compiler.compileError(token, ("Can't use field in a static method"));
return;
}
int fieldSlot = cls->findField(token);
if (atLevel<=2) compiler.writeOpArg<uint_field_index_t>(OP_LOAD_THIS_FIELD, fieldSlot);
else {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
int thisSlot = compiler.findUpvalue(thisToken, LV_FOR_READ);
compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, thisSlot);
compiler.writeOpArg<uint_field_index_t>(OP_LOAD_FIELD, fieldSlot);
}}

void FieldExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
shared_ptr<TypeInfo>* fieldType = nullptr;
if (!cls) {
compiler.compileError(token, ("Can't use field outside of a class"));
return;
}
if (compiler.getCurMethod()->flags&FD_STATIC) {
compiler.compileError(token, ("Can't use field in a static method"));
return;
}
int fieldSlot = cls->findField(token, &fieldType);
assignedValue->compile(compiler);
if (fieldType) *fieldType = compiler.mergeTypes(*fieldType, assignedValue->getType(compiler));
if (atLevel<=2) compiler.writeOpArg<uint_field_index_t>(OP_STORE_THIS_FIELD, fieldSlot);
else {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
int thisSlot = compiler.findUpvalue(thisToken, LV_FOR_READ);
compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, thisSlot);
compiler.writeOpArg<uint_field_index_t>(OP_STORE_FIELD, fieldSlot);
}}

void StaticFieldExpression::compile (QCompiler& compiler) {
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
if (!cls) {
compiler.compileError(token, ("Can't use static field oustide of a class"));
return;
}
bool isStatic = compiler.getCurMethod()->flags&FD_STATIC;
int fieldSlot = cls->findStaticField(token);
if (atLevel<=2 && !isStatic) compiler.writeOpArg<uint_field_index_t>(OP_LOAD_THIS_STATIC_FIELD, fieldSlot);
else if (atLevel<=2) {
compiler.writeOp(OP_LOAD_THIS);
compiler.writeOpArg<uint_field_index_t>(OP_LOAD_STATIC_FIELD, fieldSlot);
}
else {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
int thisSlot = compiler.findUpvalue(thisToken, LV_FOR_READ);
compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, thisSlot);
if (!isStatic) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, compiler.vm.findMethodSymbol("class"));
compiler.writeOpArg<uint_field_index_t>(OP_LOAD_STATIC_FIELD, fieldSlot);
}}

void StaticFieldExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
int atLevel = 0;
ClassDeclaration* cls = compiler.getCurClass(&atLevel);
shared_ptr<TypeInfo>* fieldType = nullptr;
if (!cls) {
compiler.compileError(token, ("Can't use static field oustide of a class"));
return;
}
bool isStatic = compiler.getCurMethod()->flags&FD_STATIC;
int fieldSlot = cls->findStaticField(token, &fieldType);
assignedValue->compile(compiler);
if (fieldType) *fieldType = compiler.mergeTypes(*fieldType, assignedValue->getType(compiler));
if (atLevel<=2 && !isStatic) compiler.writeOpArg<uint_field_index_t>(OP_STORE_THIS_STATIC_FIELD, fieldSlot);
else if (atLevel<=2) {
compiler.writeOp(OP_LOAD_THIS);
compiler.writeOpArg<uint_field_index_t>(OP_STORE_STATIC_FIELD, fieldSlot);
}
else {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
int thisSlot = compiler.findUpvalue(thisToken, LV_FOR_READ);
compiler.writeOpArg<uint_upvalue_index_t>(OP_LOAD_UPVALUE, thisSlot);
if (!isStatic) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, compiler.vm.findMethodSymbol("class"));
compiler.writeOpArg<uint_field_index_t>(OP_STORE_STATIC_FIELD, fieldSlot);
}}

void SuperExpression::compile (QCompiler& compiler) {
auto cls = compiler.getCurClass();
if (!cls) compiler.compileError(superToken, "Can't use 'super' outside of a class");
else if (cls->parents.empty()) compiler.compileError(superToken, "Can't use 'super' when having no superclass");
else {
make_shared<NameExpression>(cls->parents[0])->optimize()->compile(compiler);
compiler.writeOp(OP_LOAD_THIS); 
}}

bool LiteralSequenceExpression::isAssignable () {
if (items.size()<1) return false;
for (auto& item: items) {
shared_ptr<Expression> expr = item;
auto bop = dynamic_pointer_cast<BinaryOperation>(item);
if (bop && bop->op==T_EQ) expr = bop->left;
if (!dynamic_pointer_cast<Assignable>(expr) && !dynamic_pointer_cast<UnpackExpression>(expr)) {
if (auto cst = dynamic_pointer_cast<ConstantExpression>(expr)) return cst->token.value.i == QV::UNDEFINED.i;
else return false;
}
}
return true;
}

void LiteralSequenceExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
compiler.pushScope();
QToken tmpToken = compiler.createTempName();
auto tmpVar = make_shared<NameExpression>(tmpToken);
int slot = compiler.findLocalVariable(tmpToken, LV_NEW | LV_CONST);
assignedValue->compile(compiler);
for (int i=0, n=items.size(); i<n; i++) {
shared_ptr<Expression> item = items[i], defaultValue = nullptr;
shared_ptr<TypeInfo> typeHint = nullptr;
bool unpack = false;
auto bop = dynamic_pointer_cast<BinaryOperation>(item);
auto th = dynamic_pointer_cast<TypeHintExpression>(item);
if (bop && bop->op==T_EQ) {
item = bop->left;
defaultValue = bop->right;
th = dynamic_pointer_cast<TypeHintExpression>(defaultValue);
}
if (th) {
item = th->expr;
typeHint = th->type;
}
auto assignable = dynamic_pointer_cast<Assignable>(item);
if (!assignable) {
auto unpackExpr = dynamic_pointer_cast<UnpackExpression>(item);
if (unpackExpr) {
assignable = dynamic_pointer_cast<Assignable>(unpackExpr->expr);
unpack = true;
if (i+1!=items.size()) compiler.compileError(unpackExpr->nearestToken(), "Unpack expression must appear last in assignment expression");
}}
if (!assignable || !assignable->isAssignable()) continue;
QToken indexToken = { T_NUM, item->nearestToken().start, item->nearestToken().length, QV(static_cast<double>(i)) };
shared_ptr<Expression> index = make_shared<ConstantExpression>(indexToken);
if (unpack) {
QToken minusOneToken = { T_NUM, item->nearestToken().start, item->nearestToken().length, QV(static_cast<double>(-1)) };
shared_ptr<Expression> minusOne = make_shared<ConstantExpression>(minusOneToken);
index = BinaryOperation::create(index, T_DOTDOTDOT, minusOne);
}
vector<shared_ptr<Expression>> indices = { index };
auto subscript = make_shared<SubscriptExpression>(tmpVar, indices);
if (defaultValue) defaultValue = BinaryOperation::create(subscript, T_QUESTQUEST, defaultValue)->optimize();
else defaultValue = subscript;
if (typeHint) defaultValue = make_shared<TypeHintExpression>(defaultValue, typeHint)->optimize();
assignable->compileAssignment(compiler, defaultValue);
if (i+1<n) compiler.writeOp(OP_POP);
}
compiler.popScope();
}

bool LiteralMapExpression::isAssignable () {
if (items.size()<1) return false;
for (auto& item: items) {
shared_ptr<Expression> expr = item.second;
auto bop = dynamic_pointer_cast<BinaryOperation>(item.second);
if (bop && bop->op==T_EQ) expr = bop->left;
if (!dynamic_pointer_cast<Assignable>(expr) && !dynamic_pointer_cast<GenericMethodSymbolExpression>(expr)) return false;
}
return true;
}

void LiteralMapExpression::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
compiler.pushScope();
QToken tmpToken = compiler.createTempName();
int tmpSlot = compiler.findLocalVariable(tmpToken, LV_NEW | LV_CONST);
auto tmpVar = make_shared<NameExpression>(tmpToken);
assignedValue->compile(compiler);
bool first = true;
for (auto& item: items) {
shared_ptr<Expression> assigned = item.second, defaultValue = nullptr;
shared_ptr<TypeInfo> typeHint = nullptr;
auto bop = dynamic_pointer_cast<BinaryOperation>(assigned);
auto th = dynamic_pointer_cast<TypeHintExpression>(assigned);
if (bop && bop->op==T_EQ) {
assigned = bop->left;
defaultValue = bop->right;
th = dynamic_pointer_cast<TypeHintExpression>(defaultValue);
}
if (th) {
assigned = th->expr;
typeHint = th->type;
}
auto assignable = dynamic_pointer_cast<Assignable>(assigned);
if (!assignable) {
auto mh = dynamic_pointer_cast<GenericMethodSymbolExpression>(assigned);
if (mh) assignable = make_shared<NameExpression>(mh->token);
}
if (!assignable || !assignable->isAssignable()) continue;
if (!first) compiler.writeOp(OP_POP);
first=false;
shared_ptr<Expression> value = nullptr;
if (auto method = dynamic_pointer_cast<GenericMethodSymbolExpression>(item.first))  value = BinaryOperation::create(tmpVar, T_DOT, make_shared<NameExpression>(method->token));
else {
shared_ptr<Expression> subscript = item.first;
if (auto field = dynamic_pointer_cast<FieldExpression>(subscript))  {
field->token.value = QV(compiler.vm, field->token.start, field->token.length);
subscript = make_shared<ConstantExpression>(field->token);
}
else if (auto field = dynamic_pointer_cast<StaticFieldExpression>(subscript))  {
field->token.value = QV(compiler.vm, field->token.start, field->token.length);
subscript = make_shared<ConstantExpression>(field->token);
}
vector<shared_ptr<Expression>> indices = { subscript };
value = make_shared<SubscriptExpression>(tmpVar, indices);
}
if (defaultValue) value = BinaryOperation::create(value, T_QUESTQUEST, defaultValue)->optimize();
if (typeHint) value = make_shared<TypeHintExpression>(value, typeHint)->optimize();
assignable->compileAssignment(compiler, value);
}
compiler.popScope();
}


void UnaryOperation::compile (QCompiler& compiler) {
expr->compile(compiler);
auto type = expr->getType(compiler);
if (op==T_MINUS && type->isNum(compiler.parser.vm)) compiler.writeOp(OP_NEG);
else if (op==T_TILDE && type->isNum(compiler.parser.vm)) compiler.writeOp(OP_BINNOT);
else if (op==T_EXCL) compiler.writeOp(OP_NOT);
else compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, compiler.vm.findMethodSymbol(rules[op].prefixOpName));
}

void BinaryOperation::compile  (QCompiler& compiler) {
left->compile(compiler);
right->compile(compiler);
if (left->getType(compiler)->isNum(compiler.parser.vm) && right->getType(compiler)->isNum(compiler.parser.vm) && BASE_OPTIMIZED_OPS[op]) {
compiler.writeOp(static_cast<QOpCode>(BASE_OPTIMIZED_OPS[op]));
}
else compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_2, compiler.vm.findMethodSymbol(rules[op].infixOpName));
}

void ShortCircuitingBinaryOperation::compile (QCompiler& compiler) {
QOpCode op = this->op==T_AMPAMP? OP_AND : (this->op==T_BARBAR? OP_OR : OP_NULL_COALESCING);
left->compile(compiler);
int pos = compiler.writeOpJump(op);
right->compile(compiler);
compiler.patchJump(pos);
}

void ConditionalExpression::compile (QCompiler& compiler) {
condition->compile(compiler);
int ifJump = compiler.writeOpJump(OP_JUMP_IF_FALSY);
ifPart->compile(compiler);
int elseJump = compiler.writeOpJump(OP_JUMP);
compiler.patchJump(ifJump);
elsePart->compile(compiler);
compiler.patchJump(elseJump);
}

void SwitchExpression::compile (QCompiler& compiler) {
vector<int> endJumps;
expr->compile(compiler);
for (auto& c: cases) {
bool notFirst=false;
vector<int> condJumps;
for (auto& item: c.first) {
if (notFirst) condJumps.push_back(compiler.writeOpJump(OP_OR));
notFirst=true;
compiler.writeDebugLine(item->nearestToken());
item->compile(compiler);
}
for (auto pos: condJumps) compiler.patchJump(pos);
int ifJump = compiler.writeOpJump(OP_JUMP_IF_FALSY);
compiler.writeOp(OP_POP);
c.second->compile(compiler);
endJumps.push_back(compiler.writeOpJump(OP_JUMP));
compiler.patchJump(ifJump);
}
compiler.writeOp(OP_POP);
if (defaultCase) defaultCase->compile(compiler);
else compiler.writeOp(OP_LOAD_UNDEFINED);
for (auto pos: endJumps) compiler.patchJump(pos);
}

void SubscriptExpression::compile  (QCompiler& compiler) {
int subscriptSymbol = compiler.vm.findMethodSymbol("[]");
bool vararg = isVararg();
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
receiver->compile(compiler);
for (auto arg: args) arg->compile(compiler);
if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, subscriptSymbol);
else compiler.writeOpCallMethod(args.size(), subscriptSymbol);
}

void SubscriptExpression::compileAssignment  (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
int subscriptSetterSymbol = compiler.vm.findMethodSymbol("[]=");
bool vararg = isVararg();
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
receiver->compile(compiler);
for (auto arg: args) arg->compile(compiler);
assignedValue->compile(compiler);
vector<shared_ptr<Expression>> tmpargs = args; tmpargs.push_back(assignedValue);
//todo: update generic subtype if possible
if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, subscriptSetterSymbol);
else compiler.writeOpCallMethod(args.size() +1, subscriptSetterSymbol);
}

void MemberLookupOperation::compile (QCompiler& compiler) {
shared_ptr<SuperExpression> super = dynamic_pointer_cast<SuperExpression>(left);
shared_ptr<NameExpression> getter = dynamic_pointer_cast<NameExpression>(right);
if (getter) {
if (getter->token.type==T_END) getter->token = compiler.parser.curMethodNameToken;
int symbol = compiler.vm.findMethodSymbol(string(getter->token.start, getter->token.length));
left->compile(compiler);
type = compiler.mergeTypes(type, compiler.resolveCallType(left, getter->token, 0, nullptr, !!super));
compiler.writeOpArg<uint_method_symbol_t>(super? OP_CALL_SUPER_1 : OP_CALL_METHOD_1, symbol);
return;
}
shared_ptr<CallExpression> call = dynamic_pointer_cast<CallExpression>(right);
if (call) {
getter = dynamic_pointer_cast<NameExpression>(call->receiver);
if (getter) {
if (getter->token.type==T_END) getter->token = compiler.parser.curMethodNameToken;
int symbol = compiler.vm.findMethodSymbol(string(getter->token.start, getter->token.length));
bool vararg = call->isVararg();
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
left->compile(compiler);
call->compileArgs(compiler);
type = compiler.mergeTypes(type, compiler.resolveCallType(left, getter->token, call->args.size(), &(call->args[0]), !!super));
if (super&&vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_SUPER_VARARG, symbol);
else if (super) compiler.writeOpCallSuper(call->args.size(), symbol);
else if (vararg) compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_VARARG, symbol);
else compiler.writeOpCallMethod(call->args.size(), symbol);
return;
}}
compiler.compileError(right->nearestToken(), ("Bad operand for '.' operator"));
}

void MemberLookupOperation::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
shared_ptr<SuperExpression> super = dynamic_pointer_cast<SuperExpression>(left);
shared_ptr<NameExpression> setter = dynamic_pointer_cast<NameExpression>(right);
if (setter) {
string sName = string(setter->token.start, setter->token.length) + ("=");
int symbol = compiler.vm.findMethodSymbol(sName);
left->compile(compiler);
assignedValue->compile(compiler);
type = compiler.resolveCallType(left, setter->token, 1, &assignedValue, !!super);
compiler.writeOpArg<uint_method_symbol_t>(super? OP_CALL_SUPER_2 : OP_CALL_METHOD_2, symbol);
return;
}
compiler.compileError(right->nearestToken(), ("Bad operand for '.' operator in assignment"));
}

void MethodLookupOperation::compile (QCompiler& compiler) {
left->compile(compiler);
shared_ptr<NameExpression> getter = dynamic_pointer_cast<NameExpression>(right);
if (getter) {
int symbol = compiler.vm.findMethodSymbol(string(getter->token.start, getter->token.length));
compiler.writeOpArg<uint_method_symbol_t>(OP_LOAD_METHOD, symbol);
return;
}
compiler.compileError(right->nearestToken(), ("Bad operand for '::' operator"));
}

void MethodLookupOperation::compileAssignment (QCompiler& compiler, shared_ptr<Expression> assignedValue) {
left->compile(compiler);
shared_ptr<NameExpression> setter = dynamic_pointer_cast<NameExpression>(right);
if (setter) {
int symbol = compiler.vm.findMethodSymbol(string(setter->token.start, setter->token.length));
assignedValue->compile(compiler);
compiler.writeOpArg<uint_method_symbol_t>(OP_STORE_METHOD, symbol);
compiler.writeOp(OP_POP);
return;
}
compiler.compileError(right->nearestToken(), ("Bad operand for '::' operator in assignment"));
}

void CallExpression::compile (QCompiler& compiler) {
LocalVariable* lv = nullptr;
QV func = QV::UNDEFINED;
int globalIndex = -1;
if (auto name=dynamic_pointer_cast<NameExpression>(receiver)) {
if (compiler.findLocalVariable(name->token, LV_EXISTING | LV_FOR_READ, &lv)<0 && compiler.findUpvalue(name->token, LV_FOR_READ, &lv)<0 && (globalIndex=compiler.findGlobalVariable(name->token, LV_FOR_READ, &lv))<0 && compiler.getCurClass()) {
QToken thisToken = { T_NAME, THIS, 4, QV::UNDEFINED };
auto thisExpr = make_shared<NameExpression>(thisToken);
auto expr = BinaryOperation::create(thisExpr, T_DOT, shared_this())->optimize();
expr->compile(compiler);
type = compiler.resolveCallType(thisExpr, name->token, args.size(), &args[0]);
return;
}}
bool vararg = isVararg();
if (vararg) compiler.writeOp(OP_PUSH_VARARG_MARK);
receiver->compile(compiler);
compileArgs(compiler);
/*if (globalIndex>=0) {
auto gval = compiler.parser.vm.globalVariables[globalIndex];
QToken tmptok = { T_NAME, 0, 0, gval  };
type = compiler.resolveCallType(make_shared<ConstantExpression>(tmptok), gval, args.size(), &args[0]);
}
else */
type = compiler.resolveCallType(receiver, args.size(), &args[0]);
if (vararg) compiler.writeOp(OP_CALL_FUNCTION_VARARG);
else compiler.writeOpCallFunction(args.size());
}

void AssignmentOperation::compile (QCompiler& compiler) {
shared_ptr<Assignable> target = dynamic_pointer_cast<Assignable>(left);
if (target && target->isAssignable()) {
target->compileAssignment(compiler, right);
return;
}
compiler.compileError(left->nearestToken(), ("Invalid target for assignment"));
}




void ClassDeclaration::compile (QCompiler& compiler) {
handleAutoConstructor(compiler, fields, false);
handleAutoConstructor(compiler, staticFields, true);
for (auto decoration: decorations) decoration->compile(compiler);
struct FieldInfo {  uint_field_index_t nParents, nStaticFields, nFields; } fieldInfo = { static_cast<uint_field_index_t>(parents.size()), 0, 0 };
ClassDeclaration* oldClassDecl = compiler.curClass;
compiler.curClass = this;
int nameConstant = compiler.findConstant(QV(compiler.vm, string(name.start, name.length)));
compiler.writeDebugLine(name);
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CONSTANT, nameConstant);
for (auto& parent: parents) NameExpression(parent) .compile(compiler);
int fieldInfoPos = compiler.writeOpArg<FieldInfo>(OP_NEW_CLASS, fieldInfo);
for (auto method: methods) {
int methodSymbol = compiler.vm.findMethodSymbol(string(method->name.start, method->name.length));
compiler.parser.curMethodNameToken = method->name;
//println("Compiling %s: flags=%#0$2X", string(name.start, name.length) + "::" + string(method->name.start, method->name.length), method->flags);
compiler.curMethod = method.get();
auto func = method->compileFunction(compiler);
compiler.curMethod = nullptr;
func->name = string(name.start, name.length) + "::" + string(method->name.start, method->name.length);
compiler.writeDebugLine(method->name);
if (method->flags&FD_STATIC) compiler.writeOpArg<uint_method_symbol_t>(OP_STORE_STATIC_METHOD, methodSymbol);
else compiler.writeOpArg<uint_method_symbol_t>(OP_STORE_METHOD, methodSymbol);
compiler.writeOp(OP_POP);
}
for (auto decoration: decorations) compiler.writeOp(OP_CALL_FUNCTION_1);
if (findMethod({ T_NAME, CONSTRUCTOR, 11, QV::UNDEFINED }, true)) {
compiler.writeOp(OP_DUP);
compiler.writeOpArg<uint_method_symbol_t>(OP_CALL_METHOD_1, compiler.vm.findMethodSymbol(CONSTRUCTOR));
compiler.writeOp(OP_POP);
}
fieldInfo.nFields = fields.size();
fieldInfo.nStaticFields = staticFields.size();
compiler.patch<FieldInfo>(fieldInfoPos, fieldInfo);
compiler.curClass = oldClassDecl;
if (fields.size() >= std::numeric_limits<uint_field_index_t>::max()) compiler.compileError(nearestToken(), "Too many member fields");
if (staticFields.size() >= std::numeric_limits<uint_field_index_t>::max()) compiler.compileError(nearestToken(), "Too many static member fields");
}

void FunctionDeclaration::compileParams (QCompiler& compiler) {
vector<shared_ptr<Variable>> destructuring;
compiler.writeDebugLine(nearestToken());
for (auto& var: params) {
auto name = dynamic_pointer_cast<NameExpression>(var->name);
LocalVariable* lv = nullptr;
int slot;
if (!name) {
name = make_shared<NameExpression>(compiler.createTempName());
slot = compiler.findLocalVariable(name->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0), &lv);
var->value = var->value? BinaryOperation::create(name, T_QUESTQUESTEQ, var->value)->optimize() : name;
destructuring.push_back(var);
if (lv) lv->type = compiler.mergeTypes(var->value->getType(compiler), lv->type);
}
else {
slot = compiler.findLocalVariable(name->token, LV_NEW | ((var->flags&VD_CONST)? LV_CONST : 0), &lv);
if (var->value) {
auto value = BinaryOperation::create(name, T_QUESTQUESTEQ, var->value)->optimize();
value->compile(compiler);
compiler.writeOp(OP_POP);
if (lv) lv->type = compiler.mergeTypes(var->value->getType(compiler), lv->type);
}}
if (var->decorations.size()) {
for (auto& decoration: var->decorations) decoration->compile(compiler);
compiler.writeOpLoadLocal(slot);
for (auto& decoration: var->decorations) compiler.writeOpCallFunction(1);
compiler.writeOpStoreLocal(slot);
var->decorations.clear();
}
if (var->typeHint) {
auto typeHint = make_shared<TypeHintExpression>(name, var->typeHint)->optimize();
if (lv) lv->type = compiler.mergeTypes(typeHint->getType(compiler), lv->type);
//todo: use the type hint
//typeHint->compile(compiler);
//compiler.writeOp(OP_POP);
}
}
if (destructuring.size()) {
make_shared<VariableDeclaration>(destructuring)->optimizeStatement()->compile(compiler);
}
}

QFunction* FunctionDeclaration::compileFunction (QCompiler& compiler) {
QCompiler fc(compiler.parser);
fc.parent = &compiler;
fc.curMethod = this;
compiler.parser.curMethodNameToken = name;
compileParams(fc);
body=body->optimizeStatement();
fc.writeDebugLine(body->nearestToken());
body->compile(fc);
if (body->isExpression()) fc.writeOp(OP_POP);
QFunction* func = fc.getFunction(params.size());
compiler.result = fc.result;
func->vararg = (flags&FD_VARARG);
int funcSlot = compiler.findConstant(QV(func, QV_TAG_NORMAL_FUNCTION));
if (name.type==T_NAME) func->name = string(name.start, name.length);
else func->name = "<closure>";
if (flags&FD_FIBER) {
QToken fiberToken = { T_NAME, FIBER, 5, QV::UNDEFINED };
decorations.insert(decorations.begin(), make_shared<NameExpression>(fiberToken));
}
else if (flags&FD_ASYNC) {
QToken asyncToken = { T_NAME, ASYNC, 5, QV::UNDEFINED };
decorations.insert(decorations.begin(), make_shared<NameExpression>(asyncToken));
}
for (auto decoration: decorations) decoration->compile(compiler);
compiler.writeOpArg<uint_constant_index_t>(OP_LOAD_CLOSURE, funcSlot);
for (auto decoration: decorations) compiler.writeOp(OP_CALL_FUNCTION_1);
return func;
}

void doCompileTimeImport (QVM& vm, const string& baseFile, shared_ptr<Expression> exprRequestedFile) {
auto expr = dynamic_pointer_cast<ConstantExpression>(exprRequestedFile);
if (expr && expr->token.value.isString()) {
QFiber& f = vm.getActiveFiber();
f.import(baseFile, expr->token.value.asString());
f.pop();
}}



void DebugExpression::compile (QCompiler& compiler) {
expr->compile(compiler);
compiler.compileInfo(nearestToken(), "Type = %s", expr->getType(compiler)->toString());
}

