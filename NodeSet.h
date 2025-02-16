#pragma once
#include "BitSet.h"

using Node = int;
using NodeSet = BitSet64;
#define MAX_NODE (sizeof(NodeSet) * CHAR_BIT)

// ����ڵ㼯���ַ�����ʾ
void DumpNodeSet(NodeSet& bs);

// ��ȡλ����Ӧ�Ľڵ��б�
inline std::vector<Node> Nodes(NodeSet& s)
{
	return s.ToVector();
}

struct Edge
{
	Node from;
	Node to;
};


struct BasicBlock
{
	int index;
	NodeSet pred;  // ǰ���ڵ㼯
	NodeSet succ;  // ��̽ڵ㼯
	NodeSet domin;  // �ؾ��ڵ㼯

	// ��ȡ�������
	inline int GetSuccCount() const { return succ.GetSize(); }
	// ��ȡǰ������
	inline int GetPredCount() const { return pred.GetSize(); }
	// ��ȡ����ǰ��������������б�
	inline std::vector<Node> Pred() const { return pred.ToVector(); }
	// ��ȡ���к�̻�����������б�
	inline std::vector<Node> Succ() const { return succ.ToVector(); }
	// ��ȡָ���ڵ�����ڣ�ǰ��+��̣��ڵ㼯��
	inline std::vector<Node> Adjacent()
	{
		NodeSet total = pred | succ;
		return total.ToVector();
	}
	// ����ַ�����ʾ
	void Dump();
};

enum CtrlTreeNodeType
{
	CTNTYPE_LEAF,  // Ҷ������
	CTNTYPE_LIST,  // ���������ڵ㹹�ɵ�����
	CTNTYPE_SELF_LOOP,  // ��ѭ��
	CTNTYPE_IF,
	CTNTYPE_IF_ELSE,
	CTNTYPE_IF_OR,  // if else �ڲ����һ����
	CTNTYPE_P2LOOP
};

const TCHAR* ToString(CtrlTreeNodeType region);

struct ControlTreeNode : BasicBlock
{
	CtrlTreeNodeType type;
	// ��ͬ���Ͷ�Ӧ��ͬ���ֶ�
	union
	{
		// ��ѭ��������
		ControlTreeNode* node;
		// ������������ �� 2��ѭ��
		struct
		{
			ControlTreeNode* first;
			ControlTreeNode* second;
		} pair;
		// if ����
		struct
		{
			ControlTreeNode* condition;
			ControlTreeNode* then;
			ControlTreeNode* _else;
		} _if;
	};
};