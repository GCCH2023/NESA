#pragma once

// 数据流分析的基类
// Graph 是有向图类型
// Node 是有向图的节点类型
template<typename Graph, typename Node>
class DataFlowAnalyzer
{
public:
	DataFlowAnalyzer(){}
	virtual ~DataFlowAnalyzer(){}
	// 进行分析
	virtual void Analyze(Graph* graph, int maxIterations = 1000)
	{
		if (!graph)
			return;
		this->graph = graph;

		Initialize();

		bool isEnd = false;
		// 迭代
		int iter = 0;
		while (!isEnd && iter < maxIterations)
		{
			isEnd = true;
			for (auto node : GetAllNodes())
			{
				if (!AnalyzeNode(node))
					isEnd = false;
			}
			iter++;
		}

		Uninitialize();
	}
protected:
	// 初始化
	virtual void Initialize() {}
	// 分析节点，返回是否结束迭代，所有节点都结束才结束
	virtual bool AnalyzeNode(Node* node) = 0;
	// 反初始化
	virtual void Uninitialize() {}
	// 获取有向图中的所有节点
	virtual const std::vector<Node*>& GetAllNodes() = 0;

	Graph* graph = nullptr;
};

