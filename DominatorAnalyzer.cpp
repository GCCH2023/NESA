#include "stdafx.h"
#include "DominatorAnalyzer.h"


DominatorAnalyzer::DominatorAnalyzer(NesDataBase& db, Allocator& allocator):
TACFunctionAnalyzer(db, allocator)
{

}

DominatorAnalyzer::~DominatorAnalyzer()
{
}

void DominatorAnalyzer::Initialize()
{
	// 每个节点的支配集合初始化为所有节点（表示未确定支配关系）。
	// 入口节点的支配集合只包含自身。
	auto blocks = GetFunction()->GetBasicBlocks();
	full = (1 << blocks.size()) - 1;  // 全集
	int index = 0;
	for (auto block : blocks)
	{
		auto dom = allocator.New<Dominator>();
		block->tag = dom;
		if (!block->prevs.empty())  // 不是入口
			dom->doms = full;
		dom->index = index++;
	}
}

bool DominatorAnalyzer::AnalyzeNode(TACBasicBlock* block)
{
	// 对于每个节点，更新其支配集合为所有前驱节点的支配集合的交集，并加上自身。
	// 重复迭代，直到所有节点的支配集合不再变化。
	NodeSet doms = full;
	for (auto prev : block->prevs)
	{
		auto tag = (Dominator*)prev->tag;
		doms &= tag->doms;
	}
	auto tag = (Dominator*)block->tag;
	doms |= tag->index;
	if (doms != tag->index)
	{
		tag->doms = doms;
		return false;
	}
	return true;
}

void DominatorAnalyzer::Uninitialize()
{

}
