#include "stdafx.h"
#include "NesAnalyzer.h"
#include "NesSubroutineParser.h"
#include "NesDataBase.h"
#include "ReachingDefinition.h"
#include "LiveVariableAnalysis.h"
#include "TACTranslater.h"
#include "LiveVariableAnalysis.h"

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
	auto& subroutines = db.GetSubroutines();
	for (auto sub : subroutines)
		DumpCallRelation(sub);
}

void NesAnalyzer::AnalyzeSubroutineRegisterAXY()
{
	// 首先给所有子程序编号
	auto& subroutines = db.GetSubroutines();
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
			auto callSub = db.FindSubroutine(addr);
			sd->calls |= 1 << ((SubroutineData*)callSub->tag)->index;  // 设置子程序的调用集
		}
	}
	// 迭代分析所有子程序
	NodeSet analyzeSubs = 0;  // 已经分析过了的子程序集
	Allocator tempAllocator;
	TACTranslater tacTranslater(db, tempAllocator);
	int iter = 0;
	while (true)
	{
		NodeSet oldState = analyzeSubs;
		//printf("迭代次数 %d\n", iter++);
		// 遍历每个子程序，分析满足条件的
		for (auto sub : subroutines)
		{
			auto sd = (SubroutineData*)sub->tag;
			if ((analyzeSubs & (1 << sd->index)) == 0 && (sd->calls & analyzeSubs) == sd->calls)
			{
				// 没有分析过并且它调用的子程序都分析过了，那么可以分析这个子程序了
				auto tacSub = tacTranslater.Translate(sub);
				AnalyzeTACSubroutine(tacSub);
				sub->flag = tacSub->flag;
				analyzeSubs |= 1 << sd->index;  // 标记此子程序已经分析
			}
		}
		if (analyzeSubs == oldState)
		{
			// 判断是否全部分析完毕，也可能是存在环状调用导致分析无法进行下去
			NodeSet mask = (1 << subroutines.size()) - 1;
			if (analyzeSubs != mask)
			{
				throw Exception(_T("分析失败：分析子程序调用关系时遇到环状调用"));
			}
			break;
		}
	}
}

void NesAnalyzer::AnalyzeTACSubroutine(TACSubroutine* subroutine)
{
	auto& blocks = subroutine->GetBasicBlocks();
	// 进行活跃变量分析，以确定是否使用AXY作为参数
	LiveVariableAnalysis lva(db, allocator);
	lva.Analyze(subroutine);
	// 如果入口基本块中使用了AXY，则AXY作为参数
	auto entry = *blocks.begin();
	auto lives = (BasicBlockLiveVariableSet*)entry->tag;
	subroutine->flag = (uint32_t)lives->in.ToInteger();

	// 进行到达定值分析，如果AXY能够到达返回基本块块，则可能是返回值
	ReachingDefinition rd(db, allocator);
	rd.Analyze(subroutine);

	for (auto block : subroutine->GetBasicBlocks())
	{
		if ((block->flag & BBF_END_RETURN) != 0)
		{
			auto blockSet = (BasicBlockReachingDefinitionSet*)block->tag;
			if (blockSet->out.a.Any())
				subroutine->flag |= SUBF_RTURN_A;
			if (blockSet->out.x.Any())
				subroutine->flag |= SUBF_RTURN_X;
			if (blockSet->out.y.Any())
				subroutine->flag |= SUBF_RTURN_Y;
		}
	}
	
	DumpTACSubroutineAXY(subroutine);
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
}

bool NesAnalyzer::IsSubroutineAnalyzed(Nes::Address addr)
{
	auto it = subMap.find(addr);
	return it != subMap.end();
}
