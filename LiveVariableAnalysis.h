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

// 进行活跃变量分析
class LiveVariableAnalysis : public DataFlowAnalyzer
{
public:
	LiveVariableAnalysis(NesDataBase& db, Allocator& allocator);
protected:
	virtual void Initialize() override;

	virtual bool IteraterBasicBlock(TACBasicBlock* block) override;

private:
	Allocator& allocator;  // 用于创建输出结果
};

