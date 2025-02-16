#pragma once
#include "DataFlowAnalyzer.h"
#include "NodeSet.h"

struct BasicBlockLiveVariableSet
{
	NodeSet uses;
	NodeSet defs;
	NodeSet in;
	NodeSet out;
};

// ���л�Ծ��������
class LiveVariableAnalysis : public DataFlowAnalyzer
{
public:
	LiveVariableAnalysis(NesDataBase& db, Allocator& allocator);
protected:
	virtual void Initialize() override;

	virtual bool IteraterBasicBlock(TACBasicBlock* block) override;

private:
	Allocator& allocator;  // ���ڴ���������
};

