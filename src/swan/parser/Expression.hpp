#ifndef ___PARSER_COMPILER_EXPRESSION1
#define ___PARSER_COMPILER_EXPRESSION1
#include "StatementBase.hpp"
#include<unordered_map>

struct ConstantExpression: Expression {
QToken token;
ConstantExpression(QToken x): token(x) {}
const QToken& nearestToken () override { return token; }
void compile (QCompiler& compiler) override;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct DupExpression: Expression  {
QToken token;
DupExpression(QToken x): token(x) {}
const QToken& nearestToken () override { return token; }
void compile (QCompiler& compiler) override;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct LiteralSequenceExpression: Expression, Assignable {
QToken type;
std::vector<std::shared_ptr<Expression>> items;
LiteralSequenceExpression (const QToken& t, const std::vector<std::shared_ptr<Expression>>& p = {}): type(t), items(p) {}
const QToken& nearestToken () { return type; }
std::shared_ptr<Expression> optimize () ;
bool isVararg () ;
bool isSingleSequence ();
bool isAssignable () override;
void compileAssignment (QCompiler& compiler, std::shared_ptr<Expression> assignedValue) override;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
virtual QClass* getSequenceClass (QVM& vm) = 0;
};

struct LiteralListExpression: LiteralSequenceExpression, Functionnable {
LiteralListExpression (const QToken& t): LiteralSequenceExpression(t) {}
void makeFunctionParameters (std::vector<std::shared_ptr<struct Variable>>& params) override;
bool isFunctionnable () override;
QClass* getSequenceClass (QVM& vm) override;
void compile (QCompiler& compiler) override;
};

struct LiteralSetExpression: LiteralSequenceExpression {
LiteralSetExpression (const QToken& t): LiteralSequenceExpression(t) {}
QClass* getSequenceClass (QVM& vm) override;
void compile (QCompiler& compiler) override;
};

struct LiteralMapExpression: Expression, Assignable, Functionnable {
QToken type;
std::vector<std::pair<std::shared_ptr<Expression>, std::shared_ptr<Expression>>> items;
LiteralMapExpression (const QToken& t): type(t) {}
const QToken& nearestToken () override { return type; }
std::shared_ptr<Expression> optimize () override;
virtual void makeFunctionParameters (std::vector<std::shared_ptr<struct Variable>>& params) override;
virtual bool isFunctionnable () override;
virtual bool isAssignable () override;
virtual void compileAssignment (QCompiler& compiler, std::shared_ptr<Expression> assignedValue) override;
void compile (QCompiler& compiler) override;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct LiteralTupleExpression: LiteralSequenceExpression, Functionnable {
LiteralTupleExpression (const QToken& t, const std::vector<std::shared_ptr<Expression>>& p = {}): LiteralSequenceExpression(t, p) {}
virtual QClass* getSequenceClass (QVM& vm) override;
virtual void makeFunctionParameters (std::vector<std::shared_ptr<struct Variable>>& params) override;
virtual bool isFunctionnable () override;
virtual void compile (QCompiler& compiler) override;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct LiteralGridExpression: Expression {
QToken token;
std::vector<std::vector<std::shared_ptr<Expression>>> data;
LiteralGridExpression (const QToken& t, const std::vector<std::vector<std::shared_ptr<Expression>>>& v): token(t), data(v) {}
const QToken& nearestToken () override { return token; }
void compile (QCompiler& compiler) override;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct LiteralRegexExpression: Expression {
QToken tok;
std::string pattern, options;
LiteralRegexExpression(const QToken& tk, const std::string& p, const std::string& o): tok(tk), pattern(p), options(o) {}
const QToken& nearestToken () override { return tok; }
void compile (QCompiler& compiler) override;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct NameExpression: Expression, Assignable, Functionnable {
QToken token;
std::shared_ptr<TypeInfo> type;
NameExpression (QToken x): token(x), type(nullptr) {}
const QToken& nearestToken () override { return token; }
void compile (QCompiler& compiler) override ;
void compileAssignment (QCompiler& compiler, std::shared_ptr<Expression> assignedValue)override ;
void makeFunctionParameters (std::vector<std::shared_ptr<Variable>>& params) override;
bool isFunctionnable () override { return true; }
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override { return type; }
};

struct FieldExpression: Expression, Assignable {
QToken token;
FieldExpression (QToken x): token(x) {}
const QToken& nearestToken () override { return token; }
void compile (QCompiler& compiler)override ;
void compileAssignment (QCompiler& compiler, std::shared_ptr<Expression> assignedValue)override ;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override ;
};

struct StaticFieldExpression: Expression, Assignable {
QToken token;
StaticFieldExpression (QToken x): token(x) {}
const QToken& nearestToken () override { return token; }
void compile (QCompiler& compiler)override ;
void compileAssignment (QCompiler& compiler, std::shared_ptr<Expression> assignedValue)override ;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override ;
};

struct SuperExpression: Expression {
QToken superToken;
SuperExpression (const QToken& t): superToken(t) {}
const QToken& nearestToken () override { return superToken; }
void compile (QCompiler& compiler)  override;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct AnonymousLocalExpression: Expression, Assignable  {
QToken token;
AnonymousLocalExpression (QToken x): token(x) {}
const QToken& nearestToken () override { return token; }
void compile (QCompiler& compiler)override;
void compileAssignment (QCompiler& compiler, std::shared_ptr<Expression> assignedValue) override ;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct GenericMethodSymbolExpression: Expression {
QToken token;
GenericMethodSymbolExpression (const QToken& t): token(t) {}
const QToken& nearestToken () override { return token; }
void compile (QCompiler& compiler) override;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct BinaryOperation: Expression {
std::shared_ptr<Expression> left, right;
QTokenType op;

static std::shared_ptr<BinaryOperation> create (std::shared_ptr<Expression> left, QTokenType op, std::shared_ptr<Expression> right);

BinaryOperation (std::shared_ptr<Expression> l, QTokenType o, std::shared_ptr<Expression> r): left(l), right(r), op(o)  {}
bool isComparison () { return op>=T_LT && op<=T_GTE; }
const QToken& nearestToken () override { return left->nearestToken(); }
std::shared_ptr<Expression> optimize ()override ;
std::shared_ptr<Expression> optimizeChainedComparisons ();
void compile (QCompiler& compiler)override ;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct UnaryOperation: Expression {
std::shared_ptr<Expression> expr;
QTokenType op;
UnaryOperation  (QTokenType op0, std::shared_ptr<Expression> e0): op(op0), expr(e0) {}
std::shared_ptr<Expression> optimize ()override ;
void compile (QCompiler& compiler)override ;
const QToken& nearestToken () override { return expr->nearestToken(); }
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct ShortCircuitingBinaryOperation: BinaryOperation {
ShortCircuitingBinaryOperation (std::shared_ptr<Expression> l, QTokenType o, std::shared_ptr<Expression> r): BinaryOperation(l,o,r) {}
void compile (QCompiler& compiler)override ;
};

struct ConditionalExpression: Expression {
std::shared_ptr<Expression> condition, ifPart, elsePart;
ConditionalExpression (std::shared_ptr<Expression> cond, std::shared_ptr<Expression> ifp, std::shared_ptr<Expression> ep): condition(cond), ifPart(ifp), elsePart(ep) {}
const QToken& nearestToken () override { return condition->nearestToken(); }
std::shared_ptr<Expression> optimize () override;
void compile (QCompiler& compiler)override ;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct SwitchExpression: Expression {
std::shared_ptr<Expression> expr, var;
std::vector<std::pair<std::vector<std::shared_ptr<Expression>>, std::shared_ptr<Expression>>> cases;
std::shared_ptr<Expression> defaultCase;
const QToken& nearestToken () override { return expr->nearestToken(); }
std::shared_ptr<Expression> optimize () override;
void compile (QCompiler& compiler)override ;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct ComprehensionExpression: Expression {
std::shared_ptr<Statement> rootStatement;
std::shared_ptr<Expression> loopExpression;
ComprehensionExpression (const std::shared_ptr<Statement>& rs, const std::shared_ptr<Expression>& le): rootStatement(rs), loopExpression(le)  {}
bool isComprehension () override { return true; }
const QToken& nearestToken () override { return loopExpression->nearestToken(); }
std::shared_ptr<Expression> optimize ()override ;
void compile (QCompiler&)override ;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct UnpackExpression: Expression {
std::shared_ptr<Expression> expr;
UnpackExpression   (std::shared_ptr<Expression> e0): expr(e0) {}
bool isUnpack () override { return true; }
std::shared_ptr<Expression> optimize () override;
void compile (QCompiler& compiler) override;
const QToken& nearestToken () override { return expr->nearestToken(); }
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct TypeHintExpression: Expression, Assignable {
std::shared_ptr<Expression> expr;
std::shared_ptr<TypeInfo> type;
TypeHintExpression (std::shared_ptr<Expression> e, std::shared_ptr<TypeInfo> t): expr(e), type(t) {}
const QToken& nearestToken () override { return expr->nearestToken(); }
void compile (QCompiler& compiler)  override;
void compileAssignment (QCompiler& compiler, std::shared_ptr<Expression> assignedValue)  override;
bool isAssignable () override;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override { return type->resolve(compiler); }
};

struct AbstractCallExpression: Expression {
std::shared_ptr<Expression> receiver;
std::vector<std::shared_ptr<Expression>> args;
QTokenType callType;

AbstractCallExpression (std::shared_ptr<Expression> recv0, QTokenType tp, const std::vector<std::shared_ptr<Expression>>& args0): receiver(recv0), callType(tp), args(args0) {}
const QToken& nearestToken () override { return receiver->nearestToken(); }
std::shared_ptr<Expression> optimize () override ;
void compileArgs (QCompiler& compiler);
bool isVararg ();
};

struct CallExpression: AbstractCallExpression {
std::shared_ptr<TypeInfo> type;

CallExpression (std::shared_ptr<Expression> recv0, const std::vector<std::shared_ptr<Expression>>& args0): AbstractCallExpression(recv0, T_LEFT_PAREN, args0) {}
void compile (QCompiler& compiler)override ;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override { return type; }
};

struct SubscriptExpression: AbstractCallExpression, Assignable  {
SubscriptExpression (std::shared_ptr<Expression> recv0, const std::vector<std::shared_ptr<Expression>>& args0): AbstractCallExpression(recv0, T_LEFT_BRACKET, args0) {}
void compile (QCompiler& compiler)override ;
void compileAssignment (QCompiler& compiler, std::shared_ptr<Expression> assignedValue)override ;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct MemberLookupOperation: BinaryOperation, Assignable  {
std::shared_ptr<TypeInfo> type;

MemberLookupOperation (std::shared_ptr<Expression> l, std::shared_ptr<Expression> r): BinaryOperation(l, T_DOT, r) {}
void compile (QCompiler& compiler)override ;
void compileAssignment (QCompiler& compiler, std::shared_ptr<Expression> assignedValue)override ;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override { return type; }
};

struct MethodLookupOperation: BinaryOperation, Assignable  {
MethodLookupOperation (std::shared_ptr<Expression> l, std::shared_ptr<Expression> r): BinaryOperation(l, T_COLONCOLON, r) {}
void compile (QCompiler& compiler)override ;
void compileAssignment (QCompiler& compiler, std::shared_ptr<Expression> assignedValue)override ;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct AssignmentOperation: BinaryOperation {
bool optimized;
AssignmentOperation (std::shared_ptr<Expression> l, QTokenType o, std::shared_ptr<Expression> r): BinaryOperation(l,o,r), optimized(false)  {}
std::shared_ptr<Expression> optimize ()override ;
void compile (QCompiler& compiler)override ;
};

struct YieldExpression: Expression {
QToken token;
std::shared_ptr<Expression> expr;
YieldExpression (const QToken& tk, std::shared_ptr<Expression> e): token(tk), expr(e) {}
const QToken& nearestToken () override { return token; }
std::shared_ptr<Expression> optimize () override;
void compile (QCompiler& compiler) override;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct ImportExpression: Expression {
std::shared_ptr<Expression> from;
ImportExpression (std::shared_ptr<Expression> f): from(f) {}
std::shared_ptr<Expression> optimize () override;
const QToken& nearestToken () override { return from->nearestToken(); }
void compile (QCompiler& compiler) override;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct DebugExpression: Expression {
std::shared_ptr<Expression> expr;
DebugExpression (std::shared_ptr<Expression> e): expr(e) {}
const QToken& nearestToken () { return expr->nearestToken(); }
void compile (QCompiler& compiler);
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override { return expr->getType(compiler); }
};

struct FunctionDeclaration: Expression, Decorable {
QToken name;
std::vector<std::shared_ptr<Variable>> params;
std::shared_ptr<Statement> body;
std::shared_ptr<TypeInfo> returnTypeHint;
int flags;
int iField;

FunctionDeclaration (const QToken& nm, int fl = 0, const std::vector<std::shared_ptr<Variable>>& fp = {}, std::shared_ptr<Statement> b = nullptr):  name(nm), params(fp), body(b), returnTypeHint(nullptr), flags(fl)     {}
const QToken& nearestToken () override { return name; }
void compileParams (QCompiler& compiler);
struct QFunction* compileFunction (QCompiler& compiler);
void compile (QCompiler& compiler) override { compileFunction(compiler); }
std::shared_ptr<Expression> optimize  () override;
bool isDecorable () override { return true; }
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};

struct Field {
int index;
QToken token;
std::shared_ptr<Expression> defaultValue;
std::shared_ptr<TypeInfo> type;
};

struct ClassDeclaration: Expression, Decorable  {
QToken name;
int flags;
std::vector<QToken> parents;
std::vector<std::shared_ptr<FunctionDeclaration>> methods;
std::unordered_map<std::string, Field> fields, staticFields;

ClassDeclaration (const QToken& name0, int flgs): name(name0), flags(flgs)  {}
bool isDecorable () override { return true; }
int findField (std::unordered_map<std::string,Field>& flds, const QToken& name, std::shared_ptr<TypeInfo>** type = nullptr);
inline int findField (const QToken& name, std::shared_ptr<TypeInfo>** type = nullptr) {  return findField(fields, name, type); }
inline int findStaticField (const QToken& name, std::shared_ptr<TypeInfo>** type = nullptr) { return findField(staticFields, name, type); }
std::shared_ptr<FunctionDeclaration> findMethod (const QToken& name, bool isStatic);
void handleAutoConstructor (QCompiler& compiler, std::unordered_map<std::string,Field>& memberFields, bool isStatic);
const QToken& nearestToken () override { return name; }
std::shared_ptr<Expression> optimize () override ;
void compile (QCompiler&)override ;
std::shared_ptr<TypeInfo> getType (QCompiler& compiler) override;
};


#endif
