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

// 进行活跃变量分析
// 结果保存在基本块 tag 中，类型为 BasicBlockLiveVariableSet*
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

