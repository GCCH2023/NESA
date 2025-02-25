#include "stdafx.h"
#include "CTreeOptimizer.h"
#include "CNode.h"

CTreeOptimizer::CTreeOptimizer()
{
}


CTreeOptimizer::~CTreeOptimizer()
{
}

void CTreeOptimizer::Optimize(CNode* root)
{
	Visit(root);
}

// 使用后序遍历会比较好
void CTreeOptimizer::OnVisit(CNode* node)
{
	if (visited.find(node) != visited.end())
		return;
	visited.insert(node);

	VisitChildren(node);

	// 尝试合并列表语句
	if (node->IsStatement())
	{
		TryOptimizeStatementList(node);
		
		if (node->next)  // 语句的下一个节点必定是语句
		{
			CNode* next = node->next;  // 合并语句时，可能修改 node->next，需要先保存
			int ret = TryCombineStatementList(node, next);
			// 需要指出的是，合并两个节点后：
			// 返回 1，2的情况，第2个节点不会被访问
			// 返回 3 的情况，第1个节点被访问两次
			switch (ret)
			{
			case 1:  // 列表 + 列表
				OnVisit(next);
				node->next = next->next;
				break;
			case 2:  // 列表 + 非列表
				OnVisit(next);
				node->next = next->next;
				break;
			case 3:  // 非列表 + 列表
				// 当前节点被加入到后面的列表节点中了
				break;
			}
		}
	}
}

// 1. 两个连着的语句列表：
// 第2个列表的子语句拼接到第1个列表的子语句后面，丢弃第2个列表
// 2. 一个列表一个非列表
// 非列表作为列表的子语句添加到末尾
// 3. 非列表 + 列表
// 非列表作为列表的子语句添加到开头
int CTreeOptimizer::TryCombineStatementList(CNode* first, CNode* second)
{
	if (first->kind == CNodeKind::STAT_LIST)
	{
		if (second->kind == CNodeKind::STAT_LIST)
		{
			// 合并到末尾
			first->list.tail->next = second->list.head;
			first->list.tail = second->list.tail;
			return 1;
		}
		// 添加到末尾
		first->list.tail->next = second;
		first->list.tail = second;
		return 2;
	}
	else if (second->kind == CNodeKind::STAT_LIST)
	{
		// 添加到开头
		first->next = second->list.head;
		second->list.head = first;
		return 3;
	}
	return 0;
}

void CTreeOptimizer::TryOptimizeStatementList(CNode* node)
{
	// 对语句列表的优化
	if (node->kind != CNodeKind::STAT_LIST)
		return;

	// 1. 保证头节点不是空语句节点
	while (node->list.head && node->list.head->kind == CNodeKind::STAT_NONE)
		node->list.head = node->list.head->next;

	// 2. 如果没有子节点，那么就将这个列表节点修改为空语句节点
	if (!node->list.head)
	{
		*node = { 0 };
		node->kind = CNodeKind::STAT_NONE;
		return;
	}

	// 3. 只有一个子节点，那么用子节点代替它
	if (node->list.head->next == nullptr)
	{
		auto next = node->next;
		*node = *node->list.head;
		node->next = next;  // 保持原来的下一个节点不变
		return;
	}

	// 多个节点的情况，遍历子节点，删除空语句节点
	for (CNode* n = node->list.head; n; n = n->next)
	{
		// 头节点已经保证不是空语句节点了，所以可以不管当前节点
		while (n->next && n->next->kind == CNodeKind::STAT_NONE)
			n->next = n->next->next;
	}
}
