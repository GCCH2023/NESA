#pragma once
#include "TACFunctionAnalyzer.h"

struct Dominator
{
	NodeSet doms;  // 节点的所有支配节点
	int index;  // 节点的索引
};

// 支配节点分析
// 分析完成后，tag 为 Dominator*
class DominatorAnalyzer:
	public TACFunctionAnalyzer
{
public:
	DominatorAnalyzer(NesDataBase& db, Allocator& allocator);
	~DominatorAnalyzer();

protected:
	virtual void Initialize() override;
	virtual bool AnalyzeNode(TACBasicBlock* block) override;
	virtual void Uninitialize() override;

protected:
	NodeSet full;
};

