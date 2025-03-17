#pragma once
#include "DataFlowAnalyzer.h"
#include "NodeSet.h"

struct BasicBlockLiveVariableSet
{
	NodeSet uses;  // 使用前未定值的变量集合
	NodeSet defs;  // 定值前未使用的变量集合
	NodeSet in;  // 入口处的活跃变量集
	NodeSet out;  // 出口处的活跃变量集
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

