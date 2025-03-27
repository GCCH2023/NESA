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

	virtual CNode* TranslateBody() override;
	CNode* TranslateCall(TAC* call, CNode* params = nullptr);
	CNode* ConditionalJump(CNodeKind kind, TAC* tac);

protected:
	
};

