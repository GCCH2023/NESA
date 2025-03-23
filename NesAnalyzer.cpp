#include "stdafx.h"
#include "NesAnalyzer.h"
#include "NesSubroutineParser.h"
#include "NesDataBase.h"
#include "TACTranslater1.h"
#include "GlobalParser.h"
#include "DirectedGraph.h"
#include "TACFunctionParser.h"
#include "NesUtil.h"

struct SubroutineData
{
	size_t index;  // 在列表中的索引
	NodeSet calls;  // 调用的函数，需要先分析
};

NesAnalyzer::NesAnalyzer(NesDataBase& db_):
db(db_)
{

}

NesAnalyzer::~NesAnalyzer()
{
}


// 分析子程序
NesSubroutine* NesAnalyzer::AnalyzeSubroutine(NesSubroutineParser& parser, Nes::Address addr)
{
	parser.Reset();
	NesSubroutine* subroutine = parser.Parse(addr);

	AddSubroutine(subroutine);

	// 分析子程序使用的全局变量
	GlobalParser globalParser(db);
	globalParser.Parse(subroutine);

	// 分析完一个子程序后，接下来要分析它调用的子程序
	//printf("子程序 %04X 调用：", addr);
	for (auto addr : subroutine->GetCalls())
	{
		//printf("%04X, ", addr);
		if (IsSubroutineAnalyzed(addr))
			continue;

		AnalyzeSubroutine(parser, addr);
	}
	//printf("\n");
	//printf("分析子程序 %04X - %04X\n", subroutine->GetStartAddress(), subroutine->GetEndAddress());
	return subroutine;
}

void NesAnalyzer::DumpCallRelation(NesSubroutine* subroutine)
{
	TCHAR buffer[256];
	TCHAR* p = buffer;
	p += _stprintf_s(buffer, 256, _T("子程序 %04X - %04X 调用："), subroutine->GetStartAddress(), subroutine->GetEndAddress());
	auto& calls = subroutine->GetCalls();
	if (calls.empty())
		_stprintf_s(p, 256 - (p - buffer), _T("无\n"));
	else
	{
		for (auto call : calls)
		{
			p += _stprintf_s(p, 256 - (p - buffer), _T("%04X, "), call);
		}
		_stprintf_s(p, 256 - (p - buffer), _T("\n"));
	}
	COUT << buffer;
}

void NesAnalyzer::DumpAllCallRelation()
{
	for (auto sub : subroutines)
		DumpCallRelation(sub);
}


std::vector<NodeSet> GetStrongConnect(SubroutineList& suroutines)
{
	// 首先构造边集
	DirectedGraphEdgeList edges(32);
	edges.clear();
	for (auto sub : suroutines)
	{
		auto sd = (SubroutineData*)sub->tag;
		for (auto called : Nodes(sd->calls))
		{
			edges.push_back({ sd->index, called });
		}
	}
	DirectedGraph<int> graph(edges);
	auto vec = graph.Tarjan();
	// 删除大小为1的强连通分量列表
	auto it = std::remove_if(vec.begin(), vec.end(), [](const NodeSet& nodeSet) {
		return nodeSet.GetSize() == 1; // 删除条件：大小为 1
	});
	vec.erase(it, vec.end());
	return vec;
}


// 处理环形调用关系，返回是否成功处理
bool CanAnalyzeCycle(NodeSet analyzed, NodeSet cycle, SubroutineList& subroutines)
{
	analyzed |= cycle;  // 对于环，将构成环的所有节点当作已分析处理
	for (auto index : cycle.ToVector())
	{
		auto sub = subroutines[index];
		SubroutineData* sd = (SubroutineData*)sub->tag;
		if ((sd->calls & analyzed) != sd->calls)
			return false;  // 它调用的函数没分析完毕，那么这个环还不能够分析
	}
	return true;
}

void NesAnalyzer::AnalyzeSubroutineRegisterAXY()
{
	// 首先给所有子程序编号
	if (subroutines.size() > MAX_NODE)
	{
		Sprintf<> s;
		s.Format(_T("位集无法表示 %d 个以上的子程序"), MAX_NODE);
		throw Exception(s.ToString());  // 需要自定义类来实现
	}

	size_t index = 0;
	// 创建附加数据用于分析
	for (auto sub : subroutines)
	{
		auto sd = allocator.New<SubroutineData>();
		sd->index = index++;
		sub->tag = sd;
	}
	// 初始化子程序调用集
	for (auto sub : subroutines)
	{
		SubroutineData* sd = (SubroutineData*)sub->tag;
		for (auto addr : sub->GetCalls())
		{
			auto callSub = FindSubroutine(addr);
			sd->calls |= 1 << ((SubroutineData*)callSub->tag)->index;  // 设置子程序的调用集
		}
	}
	// 迭代分析所有子程序
	Allocator tempAllocator;

	TACTranslater1 tacTranslater(db, tempAllocator);
	TACFunctionParser tacFuncParser(db);

	// 首先计算强连通分量
	auto strongConnect = GetStrongConnect(subroutines);

	int iter = 0;
	NodeSet analyzeSubs = 0;  // 已经分析过了的子程序集
	NodeSet full((1 << subroutines.size()) - 1);
	while (true)
	{
		while (true)
		{
			NodeSet oldState = analyzeSubs;
			//printf("迭代次数 %d\n", iter++);
			// 遍历每个子程序，分析满足条件的
			for (auto sub : subroutines)
			{
				auto sd = (SubroutineData*)sub->tag;
				if (!analyzeSubs.Contains(sd->index) && (sd->calls & analyzeSubs) == sd->calls)
				{
					// 没有分析过并且它调用的子程序都分析过了，那么可以分析这个子程序了
					auto tacSub = tacTranslater.Translate(sub);
					tacFuncParser.Parse(tacSub);
					sub->flag = tacSub->flag;
					analyzeSubs += sd->index;  // 标记此子程序已经分析
				}
			}
			if (analyzeSubs == oldState)
			{
				if (analyzeSubs == full)
					return;    // 全部函数分析完毕
				break;  // 遇到环形调用
			}
		}

		// 存在环状调用的情况
		// 从后往前遍历强连通分量列表
		auto it = strongConnect.rbegin();
		for (; it != strongConnect.rend(); ++it)
		{
			if (!CanAnalyzeCycle(analyzeSubs, *it, subroutines))
				continue;

			auto indexVec = it->ToVector();
			std::vector<TACFunction*> funcs(indexVec.size());
			// 分析整个环的所有函数
			for (auto index : indexVec)
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
				for (auto func : funcs)
				{
					auto oldFlag = func->flag;
					func->flag = 0;
					tacFuncParser.Parse(func);
					if (func->flag != oldFlag)
						cycleEnd = false;
				}
			}
			// 保存结果，标记函数分析完毕
			for (auto index : indexVec)
			{
				auto tacSub = funcs[index];
				subroutines[index]->flag = tacSub->flag;
				analyzeSubs += index;
			}
			// 分析完毕后从强连通列表删除它
			strongConnect.erase((it + 1).base());
			break;  // 环处理完毕，继续单个分析
		}
		if (it == strongConnect.rend())
		{
			throw Exception(_T("函数原型分析失败：未知的错误"));
		}
	}
}

// 主要是解析 NES 中的三个中断处理程序，根据它们调用的子程序地址，
// 逐渐解析出所有的子程序代码
void NesAnalyzer::Analyze()
{
	NesSubroutineParser parser(db);

	Nes::Address addr = db.GetInterruptResetAddress();

	AnalyzeSubroutine(parser, addr);

	DumpAllCallRelation();

	// 分析完子程序的范围后，分析子程序对 AXY 寄存器的使用情况
	AnalyzeSubroutineRegisterAXY();
}

void NesAnalyzer::AddSubroutine(NesSubroutine* subroutine)
{
	subMap.insert({ subroutine->GetStartAddress(), subroutine });
	subroutines.push_back(subroutine);
}

NesSubroutine* NesAnalyzer::FindSubroutine(Nes::Address address)
{
	auto it = subMap.find(address);
	if (it != subMap.end())
		return it->second;
	return nullptr;
}

bool NesAnalyzer::IsSubroutineAnalyzed(Nes::Address addr)
{
	auto it = subMap.find(addr);
	return it != subMap.end();
}
