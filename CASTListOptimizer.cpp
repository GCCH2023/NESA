#include "stdafx.h"
#include "CASTListOptimizer.h"
#include "NodeConverter.h"

void CASTListOptimizer::Reset()
{
	visited.clear();
}

// 后序抽象语法树，有多条子语句则顺序遍历它们
void CASTListOptimizer::OnVisit(Statement* node)
{
	if (!node) return;

	// 首先处理子节点
	VisitChildren(node);

	if (!node->IsCompound())
		return;

	// 遍历列表语句的所有子节点
	auto& list = node->AsList();
	for (auto it = list.begin(); it != list.end();)
	{
		if (it->IsEmpty())
			it = list.erase(*it);  // 删除空语句
		else if (it->IsCompound())
		{
			auto pos = it++;
			list.insert(pos, *it);  // { } 的子节点上移一层
			list.erase(*pos);  // 删除 {}
		}
		else
			++it;
	}

	switch (list.size())
	{
	case 0:  // 没有元素则修改为空语句节点
		NodeConverter::ToEmptyStatement(node);
		return;
	case 1:  // 只有一条子语句，去除列表
		NodeConverter::To(node, std::move(*list.front()));
		return;
	}
}

// 1. 两个连着的语句列表：
// 第2个列表的子语句拼接到第1个列表的子语句后面，丢弃第2个列表
// 2. 一个列表一个非列表
// 非列表作为列表的子语句添加到末尾
// 3. 非列表 + 列表
// 非列表作为列表的子语句添加到开头
int CASTListOptimizer::TryCombineStatementList(Statement* first, Statement* second)
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

void CASTListOptimizer::TryOptimizeStatementList(Statement* node)
{
	// 对语句列表的优化
	if (!node->IsCompound())
		return;

	auto& list = node->AsList();
	// 1. 删除 空语句 节点
	for (auto it = list.begin(); it != list.end();)
	{
		if (it->IsEmpty())
			it = list.erase(it);
		else
			++it;
	}

	// 2. 如果没有子节点，那么就将这个列表节点修改为空语句节点
		// 3. 只有一个子节点，那么用子节点代替它
	switch (list.size())
	{
	case 0:
		NodeConverter::ToEmptyStatement(node);
		return;
	case 1:
		NodeConverter::To(node, std::move(*list.front()));
		return;
	}
}
