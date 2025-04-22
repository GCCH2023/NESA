#pragma once
#include "CNode.h"

class TACBasicBlock;
class TACOperand;
class CTranslator;
class TAC;

// 将三地址码基本块翻译为C代码
// 跳转指令不会被翻译C语句，而是保存为条件表达式和跳转目标地址
class CBasicBlockTranslator
{
public:
	CBasicBlockTranslator(CTranslator* translator);
	CNode* Translate(TACBasicBlock* block);
	CNode* GetCondition() { return condition; }
	uint32_t GetJumpTarget() { return jumpAddr; }
protected:
	CNode* GetExpression(const TACOperand& operand);
	CNode* TranslateCall(TAC* call, CNode* params);
	// 条件跳转语句翻译
	CNode* ConditionalJump(CNodeKind kind, TAC* tac, uint32_t& jumpAddr);

	CNode* UnaryExpression(CNodeKind kind, const TAC* tac);
	CNode* BinaryExpression(CNodeKind kind, const TAC* tac);
private:
	CTranslator* translator;
	CNode* condition;  // 跳转指令对应的条件表达式
	uint32_t jumpAddr;  // 跳转指令对应的
};

