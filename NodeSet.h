#pragma once
#include "BitSet.h"
#include "DynamicBitSet.h"

using Node = int;
using NodeSet = BitSet64;
#define MAX_NODE (sizeof(NodeSet) * CHAR_BIT)

// 输出节点集的字符串表示
template<typename T>
void DumpNodeSet(const T& bs)
{
	auto vec = bs.ToVector();
	if (vec.empty())
	{
		COUT << _T("空");
		return;
	}

	for (auto i : vec)
	{
		COUT << i << _T(", ");
	}
}


// 获取位集对应的节点列表
template<typename T>
inline std::vector<Node> Nodes(const T& s)
{
	return s.ToVector();
}

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

struct ControlTreeNode
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