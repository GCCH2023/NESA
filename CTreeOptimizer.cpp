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

// ʹ�ú��������ȽϺ�
void CTreeOptimizer::OnVisit(CNode* node)
{
	if (visited.find(node) != visited.end())
		return;
	visited.insert(node);

	VisitChildren(node);

	// ���Ժϲ��б����
	if (node->IsStatement())
	{
		// 1. ����б�ڵ�ֻ��һ���ӽڵ㣬��ô���ӽڵ������
		if (node->kind == CNodeKind::STAT_LIST)
		{
			if (node->list.head->next == nullptr)
			{
				auto next = node->next;
				*node = *node->list.head;
				node->next = next;  // ����ԭ������һ���ڵ㲻��
			}
		}

		if (node->next)  // ������һ���ڵ�ض������
		{
			int ret = TryCombineStatementList(node, node->next);
			CNode* listNode;
			// ��Ҫָ�����ǣ��ϲ������ڵ��
			// ���� 1��2���������2���ڵ㲻�ᱻ����
			// ���� 3 ���������1���ڵ㱻��������
			switch (ret)
			{
			case 1:  // �б� + �б�
				OnVisit(node->next);
				node->next = node->next->next;
				break;
			case 2:  // �б� + ���б�
				OnVisit(node->next);
				node->next = node->next->next;
				break;
			case 3:  // ���б� + �б�
				listNode = node->next;
				node->next = listNode->list.head;
				listNode->list.head = node;
				break;
			}
		}
	}
}

// 1. �������ŵ�����б�
// ��2���б�������ƴ�ӵ���1���б���������棬������2���б�
// 2. һ���б�һ�����б�
// ���б���Ϊ�б���������ӵ�ĩβ
// 3. ���б� + �б�
// ���б���Ϊ�б���������ӵ���ͷ
int CTreeOptimizer::TryCombineStatementList(CNode* first, CNode* second)
{
	if (first->kind == CNodeKind::STAT_LIST)
	{
		if (second->kind == CNodeKind::STAT_LIST)
		{
			// �ϲ���ĩβ
			first->list.tail->next = second->list.head;
			first->list.tail = second->list.tail;
			return 1;
		}
		// ��ӵ�ĩβ
		first->list.tail->next = second;
		first->list.tail = second;
		return 2;
	}
	else if (second->kind == CNodeKind::STAT_LIST)
	{
		// ��ӵ���ͷ
		first->next = second->list.head;
		second->list.head = first;
		return 3;
	}
	return 0;
}
