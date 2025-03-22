#pragma once
#include "TACFunctionAnalyzer.h"

// 节点集的前 TAC_ANALIZE_REG_COUNT 位用于表示寄存器变量
// 后面的位则用于表示临时变量
struct BasicBlockLiveVariableSet
{
	NodeSet uses;  // 使用前未定值的变量集合
	NodeSet defs;  // 定值前未使用的变量集合
	NodeSet in;  // 入口处的活跃变量集
	NodeSet out;  // 出口处的活跃变量集
};

// 进行活跃变量分析
// 结果保存在基本块 tag 中，类型为 BasicBlockLiveVariableSet*
class LiveVariableAnalysis:
	public TACFunctionAnalyzer
{
public:
	LiveVariableAnalysis(NesDataBase& db, Allocator& allocator);
protected:
	virtual void Initialize() override;
	virtual bool AnalyzeNode(TACBasicBlock* block) override;
};

