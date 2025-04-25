#pragma once
#include "CBasicBlockBaseTranslator.h"

// 将三地址码基本块翻译为C代码
// 跳转指令不会被翻译C语句，而是保存为条件表达式和跳转目标地址
class CBasicBlockTranslator:
	public CBasicBlockBaseTranslator
{
public:
	CBasicBlockTranslator(CTranslator* translator);
	CNode* GetCondition() { return condition; }
	uint32_t GetJumpTarget() { return jumpAddr; }
protected:
	CNode* OnTranslate(const TACBasicBlock* block) override;
	CNode* TranslateTAC(const TAC* tac, size_t& index) override;
	CNode* ConditionalJump(CNodeKind kind, const TAC* tac, uint32_t& jumpAddr);

private:
	CNode* condition;  // 跳转指令对应的条件表达式
	uint32_t jumpAddr;  // 跳转指令对应的
};

