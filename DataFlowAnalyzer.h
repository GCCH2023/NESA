#pragma once

// 数据流分析的基类
// Graph 是有向图类型
// Node 是有向图的节点类型
// Result: 数据流分析结果
template<typename Graph, typename Node, typename Result>
class DataFlowAnalyzer
{
public:
	DataFlowAnalyzer(){}
	virtual ~DataFlowAnalyzer(){}
	// 进行分析
	virtual std::shared_ptr<Result> Analyze(Graph* graph, int maxIterations = 1000)
	{
		if (!graph)
			return nullptr;
		this->graph = graph;

		Initialize();

		bool changed = true;
		int iter = 0;
		while (changed && iter < maxIterations)
		{
			changed = false;
			for (auto node : GetAllNodes())
			{
				if (AnalyzeNode(node))
					changed = true;
			}
			iter++;
		}

		Uninitialize();

		return GetResult();
	}
	// 获取分析结果
	std::shared_ptr<Result> GetResult() { return result; }
protected:
	// 初始化，返回分配的结果对象
	virtual void Initialize() {}
	// 分析节点，返回状态是否发生变化
	virtual bool AnalyzeNode(Node* node) = 0;
	// 反初始化
	virtual void Uninitialize() {}
	// 获取有向图中的所有节点
	virtual const std::vector<Node*>& GetAllNodes() = 0;
	// 设置结果
	inline void SetResult(std::shared_ptr<Result> result) { this->result = result; }

	Graph* graph = nullptr;
	std::shared_ptr<Result> result;
};

