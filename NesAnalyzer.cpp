#include "stdafx.h"
#include "NesAnalyzer.h"
#include "SubroutineParser.h"
#include "NesDataBase.h"
#include "TACTranslater1.h"
#include "GlobalParser.h"
#include "TACFunctionParser.h"
#include "NesUtil.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/strong_components.hpp>

using namespace boost;

// 自定义节点属性
struct NodeProperty
{
	NesSubroutine* subroutine;
	bool analyzed;
};

// 图类型定义
typedef adjacency_list<vecS, vecS, directedS, NodeProperty> Graph;
typedef graph_traits<Graph>::vertex_descriptor Vertex;
typedef graph_traits<Graph>::edge_descriptor Edge;
typedef adjacency_list<boost::vecS, boost::vecS, boost::directedS> ComponentGraph;

// NES 子程序原型分析
// 即分析子程序的参数和返回值
class NesSubroutineProtoAnalyzer
{
	NesDataBase& db;
	Graph graph;
	SubroutineList& subroutines;
	Allocator tempAllocator;
	TACTranslater1 tacTranslater;
	TACFunctionParser tacFuncParser;

	void AnalyzeFunction(Vertex v)
	{
		if (graph[v].analyzed)
			return;

		auto sub = graph[v].subroutine;
		auto tacSub = tacTranslater.Translate(sub);
		tacFuncParser.Parse(tacSub);
		sub->flag = tacSub->flag;

		graph[v].analyzed = true;
	}

	void AnalyzeSCC(const std::vector<Vertex>& scc)
	{
		std::vector<TACFunction*> funcs(num_vertices(graph), nullptr);
		// 分析整个环的所有函数
		for (auto index : scc)
		{
			auto sub = subroutines[index];
			auto tacSub = tacTranslater.Translate(sub);
			tacFuncParser.Parse(tacSub);
			funcs[index] = tacSub;
		}
		// 迭代分析，直到所有函数的返回值和参数保持不变
		bool cycleEnd = false;
		while (!cycleEnd)
		{
			cycleEnd = true;
			for (auto i : scc)
			{
				auto func = funcs[i];
				auto oldFlag = func->flag;
				func->flag = 0;
				tacFuncParser.Parse(func);
				if (func->flag != oldFlag)
					cycleEnd = false;
			}
		}
		// 保存结果，标记函数分析完毕
		for (auto index : scc)
		{
			auto tacSub = funcs[index];
			subroutines[index]->flag = tacSub->flag;
			graph[index].analyzed = true;  // 标记为已分析
		}
	}

public:
	NesSubroutineProtoAnalyzer(NesDataBase& db_, SubroutineList& subroutines_):
		db(db_),
		subroutines(subroutines_),
		graph(subroutines_.size()),
		tacTranslater(db, tempAllocator),
		tacFuncParser(db)
	{
	}

	void Analyze()
	{
		// 绑定节点和函数
		for (size_t i = 0; i < subroutines.size(); ++i)
		{
			graph[i].subroutine = subroutines[i];
			graph[i].analyzed = false;
			subroutines[i]->tag = (void*)i;
		}

		// 函数调用关系构成边
		for (size_t i = 0; i < subroutines.size(); ++i)
		{
			for (auto call : subroutines[i]->GetCalls())
			{
				auto callSub = db.FindSubroutine(call);
				add_edge(i, (Vertex)callSub->tag, graph);
			}
		}

		// 检测强连通分量
		std::vector<int> component(num_vertices(graph));
		int num_scc = strong_components(graph,
			make_iterator_property_map(component.begin(), get(vertex_index, graph)));

		// 按组件分组
		std::vector<std::vector<Vertex>> scc_groups(num_scc);
		for (size_t i = 0; i < component.size(); ++i)
		{
			scc_groups[component[i]].push_back(vertex(i, graph));
		}

		// 构建组件图（将每个SCC视为一个顶点）
		ComponentGraph component_graph(num_scc);
		for (auto edge_it = edges(graph); edge_it.first != edge_it.second; ++edge_it.first)
		{
			Vertex u = source(*edge_it.first, graph);
			Vertex v = target(*edge_it.first, graph);
			if (component[u] != component[v])
			{
				add_edge(component[u], component[v], component_graph);
			}
		}

		// 对组件进行拓扑排序
		std::vector<ComponentGraph::vertex_descriptor> component_order;
		topological_sort(component_graph, std::back_inserter(component_order));
		std::reverse(component_order.begin(), component_order.end());

		// 存储分析结果
		for (size_t comp_id : component_order)
		{
			const auto& scc = scc_groups[comp_id];

			if (scc.size() == 1)
			{
				// 单个函数，无循环依赖
				Vertex v = scc[0];
				AnalyzeFunction(v);
			}
			else
			{
				// 循环依赖组，需要迭代分析
				AnalyzeSCC(scc);
			}
		}
	}
};


NesAnalyzer::NesAnalyzer(NesDataBase& db_):
db(db_)
{

}

NesAnalyzer::~NesAnalyzer()
{
}


// 分析子程序
void NesAnalyzer::AnalyzeSubroutine()
{
	//NesSubroutinesParser parser(db);
	//this->subroutines = parser.Parse(db.GetInterruptResetAddress());

	NesDB::SubroutineParser parser(db);
	// 构建初始的待分析子程地址序队列
	std::vector<Nes::Address> queue =
	{
		db.GetInterruptResetAddress(),
		db.GetInterruptNmiAddress(),
		db.GetInterruptIrqAddress(),
	};
	std::unordered_set<Nes::Address> visited;

	// 循环分析所有函数
	while (!queue.empty())
	{
		// 取出一个函数地址进行分析
		auto addr = *queue.rbegin();
		queue.pop_back();
		if (visited.find(addr) != visited.end())
			continue;

		// parser 会判断子程序是否分析过，所以这里不需要判断
		NesSubroutine* subroutine = parser.Parse(addr);
		visited.insert(addr);

		// 将子程序调用的子程序添加到队列
		for (auto call : subroutine->GetCalls())
		{
			queue.push_back(call);
		}
	}
	this->subroutines = db.GetSubroutines();
}

void NesAnalyzer::DumpCallRelation(NesSubroutine* subroutine)
{
	Sprintf<> s;
	s.Format(_T("子程序 %04X - %04X 调用："), subroutine->GetStartAddress(), subroutine->GetEndAddress());
	auto& calls = subroutine->GetCalls();
	if (calls.empty())
		s.Append(_T("无\n"));
	else
	{
		for (auto call : calls)
		{
			s.Append(_T("%04X, "), call);
		}
		s.Erase(2);
		s.Append(_T("\n"));
	}
	COUT << s.ToString();
}

void NesAnalyzer::DumpAllCallRelation()
{
	for (auto sub : GetSubroutines())
		DumpCallRelation(sub);
}

void NesAnalyzer::AnalyzeSubroutineRegisterAXY()
{
	NesSubroutineProtoAnalyzer analyzer(db, GetSubroutines());
	analyzer.Analyze();
}

// 主要是解析 NES 中的三个中断处理程序，根据它们调用的子程序地址，
// 逐渐解析出所有的子程序代码
void NesAnalyzer::Analyze()
{
	AnalyzeSubroutine();

	// 分析子程序使用的全局变量
	GlobalParser globalParser(db);
	for (auto subroutine : GetSubroutines())
	{
		globalParser.Parse(subroutine);
	}

	DumpAllCallRelation();

	// 分析完子程序的范围后，分析子程序对 AXY 寄存器的使用情况
	AnalyzeSubroutineRegisterAXY();
}


NesSubroutine* NesAnalyzer::FindSubroutine(Nes::Address address)
{
	return db.FindSubroutine(address);
}
