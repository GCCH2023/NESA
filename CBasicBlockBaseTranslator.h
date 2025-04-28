#pragma once
#include "CNode.h"
class TAC;
class TACBasicBlock;
class TACOperand;
class CTranslator;
class CNodeFactory;

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
	virtual Statement* OnTranslate(const TACBasicBlock* block) = 0;
	// 传入当前要翻译的三地址码和它对应的索引
	virtual Statement* TranslateTAC(const TAC* tac, size_t& index);
	// 翻译双目表达式
	virtual Expression* BinaryExpression(CNodeKind kind, Expression* x, Expression* y);
	// 翻译双目赋值语句 z = x op y;
	virtual Statement* BinAssignStatement(CNodeKind kind, const TAC* tac);
	// 翻译单目赋值语句 z = op x
	virtual Statement* UnaryAssignStatement(CNodeKind kind, const TAC* tac);
	// 翻译赋值语句 z = x
	virtual Statement* AssignStatement(const TACOperand& z, Expression* x);
	// 获取三地址码操作数对应的变量
	const Variable* GetVariable(const TACOperand& operand);
	// 获取三地址码操作数对应的表达式
	virtual Expression* GetExpression(const TACOperand& operand);
	// 翻译字段表达式
	virtual Expression* FieldExpression(Expression* obj, const Field* field);
	// 翻译数组元素表达式
	virtual Expression* IndexExpression(Expression* array, Expression* index);
	// 翻译数组元素赋值或对象字段赋值语句
	virtual Statement* ArrayAssign(Expression* z, Expression* x);
	// 翻译函数调用
	virtual Statement* TranslateCall(const TAC* call, const std::vector<Expression*>& args);

	const TACBasicBlock* GetBasicBlock() const { return block; }
	CTranslator* GetTranslator() const { return translator; }
	CNodeFactory& GetNodeFactory();
	inline void SetJumpCondition(Expression* condition) { this->condition = condition; }
	inline void SetJumpTarget(uint32_t jumpTarget) { jumpAddr = jumpTarget; }
private:
	CTranslator* translator;
	const TACBasicBlock* block = nullptr;

	Expression* condition = nullptr;  // 跳转指令对应的条件表达式
	uint32_t jumpAddr = 0;  // 跳转指令对应的
};

