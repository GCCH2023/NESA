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
		TryOptimizeStatementList(node);
		
		if (node->next)  // ������һ���ڵ�ض������
		{
			CNode* next = node->next;  // �ϲ����ʱ�������޸� node->next����Ҫ�ȱ���
			int ret = TryCombineStatementList(node, next);
			// ��Ҫָ�����ǣ��ϲ������ڵ��
			// ���� 1��2���������2���ڵ㲻�ᱻ����
			// ���� 3 ���������1���ڵ㱻��������
			switch (ret)
			{
			case 1:  // �б� + �б�
				OnVisit(next);
				node->next = next->next;
				break;
			case 2:  // �б� + ���б�
				OnVisit(next);
				node->next = next->next;
				break;
			case 3:  // ���б� + �б�
				// ��ǰ�ڵ㱻���뵽������б�ڵ�����
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

void CTreeOptimizer::TryOptimizeStatementList(CNode* node)
{
	// ������б���Ż�
	if (node->kind != CNodeKind::STAT_LIST)
		return;

	// 1. ��֤ͷ�ڵ㲻�ǿ����ڵ�
	while (node->list.head && node->list.head->kind == CNodeKind::STAT_NONE)
		node->list.head = node->list.head->next;

	// 2. ���û���ӽڵ㣬��ô�ͽ�����б�ڵ��޸�Ϊ�����ڵ�
	if (!node->list.head)
	{
		*node = { 0 };
		node->kind = CNodeKind::STAT_NONE;
		return;
	}

	// 3. ֻ��һ���ӽڵ㣬��ô���ӽڵ������
	if (node->list.head->next == nullptr)
	{
		auto next = node->next;
		*node = *node->list.head;
		node->next = next;  // ����ԭ������һ���ڵ㲻��
		return;
	}

	// ����ڵ������������ӽڵ㣬ɾ�������ڵ�
	for (CNode* n = node->list.head; n; n = n->next)
	{
		// ͷ�ڵ��Ѿ���֤���ǿ����ڵ��ˣ����Կ��Բ��ܵ�ǰ�ڵ�
		while (n->next && n->next->kind == CNodeKind::STAT_NONE)
			n->next = n->next->next;
	}
}
