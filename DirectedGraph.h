#pragma once
#include "NodeSet.h"

// 有向图节点
template<typename T>
struct DirectedGraphNode
{
	Node index;
	NodeSet pred;  // 前驱节点集
	NodeSet succ;  // 后继节点集
	T tag;  // 扩展使用

	// 获取后继数量
	inline int GetSuccCount() const { return succ.GetSize(); }
	// 获取前驱数量
	inline int GetPredCount() const { return pred.GetSize(); }
	// 获取所有前驱基本块的索引列表
	inline std::vector<Node> Pred() const { return pred.ToVector(); }
	// 获取所有后继基本块的索引列表
	inline std::vector<Node> Succ() const { return succ.ToVector(); }
	// 获取指定节点的相邻（前驱+后继）节点集合
	inline std::vector<Node> Adjacent() const
	{
		NodeSet total = pred | succ;
		return total.ToVector();
	}
};

// 有向图的边
struct DirectedGraphEdge
{
	Node source;
	Node target;
};
using DirectedGraphEdgeList = std::vector<DirectedGraphEdge>;

// 有向图
template<typename T>
class DirectedGraph
{
public:
	DirectedGraph::DirectedGraph(const DirectedGraphEdgeList& edges):
		nodes(MAX_NODE),
		nodesCount(0)
	{
		Build(edges);
	}

	~DirectedGraph() = default;
	// 根据边集构建有向图
	void Build(const DirectedGraphEdgeList& edges)
	{
		for (auto& edge : edges)
		{
			Node source = edge.source;
			Node target = edge.target;
			nodes[source].succ |= 1 << target;
			nodes[target].pred |= 1 << source;

			if (nodesCount < source)
				nodesCount = source;
			if (nodesCount < target)
				nodesCount = target;
		}
		++nodesCount;  // 数量 = 最大索引 + 1
		for (int i = 0; i < nodesCount; ++i)
		{
			nodes[i].index = i;
		}
	}
	// 获取所有节点构成的全集
	inline NodeSet GetFullSet() const
	{
		return (1 << nodesCount) - 1;
	}
	// 获取指定索引的节点
	inline DirectedGraphNode& operator[](size_t index)
	{
		return nodes[index];
	}
protected:
	std::vector<DirectedGraphNode<T>> nodes;
	int nodesCount;  // 节点数量
};
