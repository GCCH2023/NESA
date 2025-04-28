#pragma once
#include "CTranslator.h"
#include "CNode.h"

class Function;
class TACFunction;
class TAC;

// 将三地址码的函数直接翻译成C函数
// 不考虑控制关系，全部跳转指令都翻译为goto语句
class CDirectTranslator :
	public CTranslator
{
public:
	CDirectTranslator(Allocator& allocator);
	~CDirectTranslator();

	virtual void Reset() override;
protected:

	virtual Statement* TranslateBody() override;
	virtual BasicBlockResult TranslateBasicBlock(const TACBasicBlock* block) override;

	Statement* TranslateCall(TAC* call, const std::vector<Expression*>& args);
	Statement* ConditionalJump(CNodeKind kind, TAC* tac);
	Statement* ConditionalJump(const BasicBlockResult& ret);

	Statement* UnaryExpression(CNodeKind kind, const TAC* tac);
	Statement* BinaryExpression(CNodeKind kind, const TAC* tac);
protected:
	
};

