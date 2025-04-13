#pragma once
#include "NodeSet.h"

using VertexSet = DynamicBitSet;

// 有向图节点
template<typename T>
struct DirectedGraphNode : public T
{
	Node index;
	VertexSet pred;  // 前驱节点集
	VertexSet succ;  // 后继节点集

	// 获取后继数量
	inline int GetSuccCount() const { return succ.Count(); }
	// 获取前驱数量
	inline int GetPredCount() const { return pred.Count(); }
	// 获取所有前驱基本块的索引列表
	inline std::vector<Node> Pred() const { return pred.ToVector(); }
	// 获取所有后继基本块的索引列表
	inline std::vector<Node> Succ() const { return succ.ToVector(); }
	// 获取指定节点的相邻（前驱+后继）节点集合
	inline std::vector<Node> Adjacent() const
	{
		VertexSet total = pred | succ;
		return total.ToVector();
	}
	void Resize(size_t size)
	{
		pred.Resize(size);
		succ.Resize(size);
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
	DirectedGraph() :nodesCount(0) {}
	DirectedGraph(const DirectedGraphEdgeList& edges)
	{
		AddEdges(edges);
	}

	~DirectedGraph() = default;
	// 清空数据
	void Clear()
	{
		nodes.clear();
		nodesCount = 0;
	}
	// 添加边集
	void AddEdges(const DirectedGraphEdgeList& edges)
	{
		for (auto& edge : edges)
		{
			Node source = edge.source;
			Node target = edge.target;
			nodes[source].succ += target;
			nodes[target].pred += source;
		}
		for (int i = 0; i < nodesCount; ++i)
		{
			nodes[i].index = i;
		}
	}
	// 获取所有节点构成的全集
	inline VertexSet GetFullSet() const
	{
		return VertexSet::FullSet(nodesCount);
	}
	// 获取指定索引的节点
	inline DirectedGraphNode<T>& operator[](size_t index)
	{
		return nodes[index];
	}
	inline DirectedGraphNode<T>* GetNode(size_t index)
	{
		return &nodes[index];
	}
	// 获取节点数量
	inline int GetNodeCount() const { return nodesCount; }
	// 使用 Tarjan 算法获取有向图的强连通风量
	// 每个VertexSet表示一个强连通分量
	std::vector<VertexSet> Tarjan() const
	{
		std::vector<VertexSet> sccs;  // 存储所有强连通分量
		std::vector<int> dfn(nodesCount, -1);  // DFS 访问顺序
		std::vector<int> low(nodesCount, -1);  // 最小可达节点
		std::stack<Node> stack;  // 用于存储当前路径上的节点
		std::vector<bool> inStack(nodesCount, false);  // 标记节点是否在栈中
		int index = 0;  // DFS 访问顺序计数器

		// 对所有未访问的节点调用 StrongConnect
		for (Node v = 0; v < nodesCount; ++v) {
			if (dfn[v] == -1) {
				StrongConnect(v, &dfn[0], low, stack, inStack, index, sccs);
			}
		}

		return sccs;
	}
	// 设置节点数量
	void Resize(size_t size)
	{
		nodesCount = size;
		nodes.resize(nodesCount);
		for (auto& n : nodes)
		{
			n.Resize(nodesCount);
		}
	}
protected:
	
	void StrongConnect(Node v, int dfn[],
		std::vector<int>& low,
		std::stack<Node>& stack,
		std::vector<bool>& inStack,
		int& index,
		std::vector<VertexSet>& sccs) const
	{
		dfn[v] = low[v] = index++;
		stack.push(v);
		inStack[v] = true;

		// 遍历所有后继节点
		for (Node w : nodes[v].Succ()) {
			if (dfn[w] == -1) {  // 如果未访问过
				StrongConnect(w, dfn, low, stack, inStack, index, sccs);
				low[v] = std::min(low[v], low[w]);
			}
			else if (inStack[w]) {  // 如果已在栈中
				low[v] = std::min(low[v], dfn[w]);
			}
		}

		// 如果 v 是强连通分量的根节点
		if (low[v] == dfn[v]) {
			VertexSet scc;
			Node w;
			do {
				w = stack.top();
				stack.pop();
				inStack[w] = false;
				scc += w;
			} while (w != v);
			sccs.push_back(scc);
		}
	}
protected:
	std::vector<DirectedGraphNode<T>> nodes;
	size_t nodesCount;  // 节点数量
};
