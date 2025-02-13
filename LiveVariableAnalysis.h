#pragma once
#include "DataFlowAnalyzer.h"
#include "NodeSet.h"

struct BasicBlockLiveVariableSet
{
	BitSet32 uses;
	BitSet32 defs;
	BitSet32 in;
	BitSet32 out;
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

