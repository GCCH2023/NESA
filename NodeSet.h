#pragma once
#include "BitSet.h"

using Node = int;
using NodeSet = BitSet64;
#define MAX_NODE (sizeof(NodeSet) * CHAR_BIT)

// 输出节点集的字符串表示
void DumpNodeSet(NodeSet& bs);

// 获取位集对应的节点列表
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
	NodeSet pred;  // 前驱节点集
	NodeSet succ;  // 后继节点集
	NodeSet domin;  // 必经节点集

	// 获取后继数量
	inline int GetSuccCount() const { return succ.GetSize(); }
	// 获取前驱数量
	inline int GetPredCount() const { return pred.GetSize(); }
	// 获取所有前驱基本块的索引列表
	inline std::vector<Node> Pred() const { return pred.ToVector(); }
	// 获取所有后继基本块的索引列表
	inline std::vector<Node> Succ() const { return succ.ToVector(); }
	// 获取指定节点的相邻（前驱+后继）节点集合
	inline std::vector<Node> Adjacent()
	{
		NodeSet total = pred | succ;
		return total.ToVector();
	}
	// 输出字符串表示
	void Dump();
};

enum CtrlTreeNodeType
{
	CTNTYPE_LEAF,  // 叶子区域
	CTNTYPE_LIST,  // 两个连续节点构成的区域
	CTNTYPE_SELF_LOOP,  // 自循环
	CTNTYPE_IF,
	CTNTYPE_IF_ELSE,
	CTNTYPE_IF_OR,  // if else 内部多出一条边
	CTNTYPE_P2LOOP
};

const TCHAR* ToString(CtrlTreeNodeType region);

struct ControlTreeNode : BasicBlock
{
	CtrlTreeNodeType type;
	// 不同类型对应不同的字段
	union
	{
		// 自循环的区域
		ControlTreeNode* node;
		// 连续两个区域 或 2点循环
		struct
		{
			ControlTreeNode* first;
			ControlTreeNode* second;
		} pair;
		// if 区域
		struct
		{
			ControlTreeNode* condition;
			ControlTreeNode* then;
			ControlTreeNode* _else;
		} _if;
	};
};