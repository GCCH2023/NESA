#pragma once
#include "CNode.h"
class TAC;
class TACBasicBlock;
class TACOperand;
class CTranslator;
class NodeFactory;

class CBasicBlockBaseTranslator
{
public:
	CBasicBlockBaseTranslator(CTranslator* translator_) :
		translator(translator_)
	{
	}
	virtual ~CBasicBlockBaseTranslator() = default;

protected:
	// 传入当前要翻译的三地址码和它对应的索引
	virtual CNode* TranslateTAC(const TAC* tac, size_t& index);
	// 翻译双目表达式
	virtual CNode* BinaryExpression(CNodeKind kind, CNode* x, CNode* y);
	// 翻译双目赋值语句 z = x op y;
	virtual CNode* BinAssignStatement(CNodeKind kind, const TAC* tac);
	// 翻译单目赋值语句 z = op x
	virtual CNode* UnaryAssignStatement(CNodeKind kind, const TAC* tac);
	// 翻译赋值语句 z = x
	virtual CNode* AssignStatement(const TACOperand& z, CNode* x);
	// 获取三地址码操作数对应的变量
	const Variable* GetVariable(const TACOperand& operand);
	// 获取三地址码操作数对应的表达式
	virtual CNode* GetExpression(const TACOperand& operand);
	// 翻译字段表达式
	virtual CNode* FieldExpression(CNode* obj, const Field* field);
	// 翻译数组元素表达式
	virtual CNode* IndexExpression(CNode* array, CNode* index);
	// 翻译数组元素赋值或对象字段赋值
	virtual CNode* ArrayAssign(CNode* z, CNode* x);

	const TACBasicBlock* GetBasicBlock() const { return block; }
	CTranslator* GetTranslator() const { return translator; }
	CNodeFactory& GetNodeFactory();
private:
	CTranslator* translator;
	const TACBasicBlock* block = nullptr;
};

