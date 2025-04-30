#pragma once
#include "CNode.h"
class TAC;
class TACBasicBlock;
class TACOperand;
class CTranslator;
class CNodeFactory;

using ConstArgList = const std::vector<Expression*>;

// 将三地址码基本块翻译为C语句
// 基本块也就是若干条表达式+至多一条跳转指令
class CBasicBlockBaseTranslator
{
public:
	CBasicBlockBaseTranslator(CTranslator* translator_) :
		translator(translator_)
	{
	}
	virtual ~CBasicBlockBaseTranslator() = default;
	Statement* Translate(const TACBasicBlock* block);

	Expression* GetJumpCondition() { return condition; }
	uint32_t GetJumpTarget() { return jumpAddr; }
protected:
	virtual Statement* OnTranslate(const TACBasicBlock* block);
	// 传入当前要翻译的三地址码和它对应的索引，返回翻译结果表达式
	// 如果当前三地址码不能或不需要翻译为表达式，则返回nullptr
	virtual Expression* TranslateTAC(const TAC* tac, size_t& index);
	// 翻译单目表达式
	virtual Expression* UnaryExpression(CNodeKind kind, Expression* x);
	// 翻译双目表达式
	virtual Expression* BinaryExpression(CNodeKind kind, Expression* x, Expression* y);
	// 翻译赋值表达式 z = x
	virtual Expression* AssignExpression(const TACOperand& z, Expression* x);
	// 翻译数组元素赋值或对象字段赋值语句
	virtual Expression* ArrayAssign(Expression* z, Expression* x);
	// 翻译函数调用表达式 result = func(args)，需要判断是否接收返回值
	virtual Expression* CallExpression(String* func, ConstArgList& args, const TACOperand& result);
	// 获取三地址码操作数对应的表达式
	virtual Expression* GetExpression(const TACOperand& operand);
	// 翻译字段表达式
	virtual Expression* FieldExpression(Expression* obj, const Field* field);
	// 翻译数组元素表达式
	virtual Expression* IndexExpression(Expression* array, Expression* index);
	// 翻译函数调用
	virtual Expression* TranslateCall(const TAC* call, ConstArgList& args);
	// 翻译类型转换表达式
	virtual Expression* CastExpression(const Type* type, Expression* value);
	// 处理条件跳转
	virtual void ConditionalJump(CNodeKind op, Expression* x, Expression* y, uint32_t jump);
	void ConditionalJump(CNodeKind op, const TAC* tac);
	// 翻译返回语句，返回值可能为空
	virtual void Return(Expression* value);

	// 翻译单目赋值表达式 z = op x
	Expression* UnaryAssignExpression(CNodeKind kind, const TAC* tac);
	// 翻译双目赋值表达式 z = x op y;
	Expression* BinaryAssignExpression(CNodeKind kind, const TAC* tac);
	// 获取三地址码操作数对应的变量
	const Variable* GetVariable(const TACOperand& operand);
	// 不知道怎么翻译的就翻译为指定名称函数调用
	Expression* Call(const TCHAR* func, std::initializer_list<Expression*> args, const TACOperand& result);

	const TACBasicBlock* GetBasicBlock() const { return block; }
	CTranslator* GetTranslator() const { return translator; }
	CNodeFactory& GetNodeFactory();
	inline void SetJumpCondition(Expression* condition) { this->condition = condition; }
	inline void SetJumpTarget(uint32_t jumpTarget) { jumpAddr = jumpTarget; }
	inline void SetReturnStatement(Statement* stat) { returnStat = stat; }
	inline Statement* GetReturnStatement() const { return returnStat; }
private:
	CTranslator* translator;
	const TACBasicBlock* block = nullptr;

	Expression* condition = nullptr;  // 跳转指令对应的条件表达式
	uint32_t jumpAddr = 0;  // 跳转指令对应的
	Statement* returnStat = nullptr;  // 也可能是以 return 结束的
};

