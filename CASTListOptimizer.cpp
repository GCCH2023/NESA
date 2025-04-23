#include "stdafx.h"
#include "CASTListOptimizer.h"
#include "CNode.h"


void CASTListOptimizer::Reset()
{
	visited.clear();
}

#include "Dump.h"

// 后序抽象语法树，有多条子语句则顺序遍历它们
void CASTListOptimizer::OnVisit(CNode* node)
{
	if (!node) return;

	// 首先处理子节点
	VisitChildren(node);

	if (!node->IsCompoundStatement())
		return;

	// 遍历列表语句的所有子节点
	CListNode list(node);
	for (auto it = list.begin(); it != list.end();)
	{
		if (it->IsEmptyStatement())
			it = list.Remove(*it);  // 删除空语句
		else if (it->IsCompoundStatement())
		{
			auto pos = it++;
			CListNode childList(*pos);
			list.Insert(pos, childList);  // { } 的子节点上移一层
			list.Remove(*pos);  // 删除 {}
		}
		else
			++it;
	}

	switch (list.Count())
	{
	case 0:  // 没有元素则修改为空语句节点
		node->EmptyStat();
		return;
	case 1:  // 只有一条子语句，去除列表
		node->CopyData(**list.begin());
		return;
	}
}

// 1. 两个连着的语句列表：
// 第2个列表的子语句拼接到第1个列表的子语句后面，丢弃第2个列表
// 2. 一个列表一个非列表
// 非列表作为列表的子语句添加到末尾
// 3. 非列表 + 列表
// 非列表作为列表的子语句添加到开头
int CASTListOptimizer::TryCombineStatementList(CNode* first, CNode* second)
{
	//if (first->kind == CNodeKind::STAT_LIST)
	//{
	//	if (second->kind == CNodeKind::STAT_LIST)
	//	{
	//		// 合并到末尾
	//		CListNode sec(second);
	//		CListNode(first).Join(sec);
	//		return 1;
	//	}
	//	// 添加到末尾
	//	second->RemoveFromList();
	//	CListNode(first).Add(second);
	//	return 2;
	//}
	//else if (second->kind == CNodeKind::STAT_LIST)
	//{
	//	// 添加到开头
	//	second->RemoveFromList();
	//	CListNode(first).PushFront(second);
	//	return 3;
	//}
	return 0;
}

void CASTListOptimizer::TryOptimizeStatementList(CNode* node)
{
	// 对语句列表的优化
	if (node->kind != CNodeKind::STAT_LIST)
		return;

	CListNode list(node);
	// 1. 删除 空语句 节点
	for (auto it = list.begin(); it != list.end();)
	{
		if (it->IsEmptyStatement())
			it = list.Remove(*it);
		else
			++it;
	}
	while (node->list.head && node->list.head->kind == CNodeKind::STAT_EMPTY)
		node->list.head = node->list.head->GetNext();

	// 2. 如果没有子节点，那么就将这个列表节点修改为空语句节点
	if (list.Empty())
	{
		node->Reset();
		return;
	}

	// 3. 只有一个子节点，那么用子节点代替它
	if (list.Count() == 1)
	{
		node->CopyData(**list.begin());
		return;
	}
}
