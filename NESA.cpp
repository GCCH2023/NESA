// NESA.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "NesDataBase.h"
using namespace std;
using namespace Nes;

#include "NesSubroutineParser.h"
#include "TACTranslater.h"
#include "CTranslater.h"

#include "ReachingDefinition.h"
#include "LiveVariableAnalysis.h"
#include "NesAnalyzer.h"
#include "TACPeephole.h"
#include "TACDeadCodeElimination.h"

void ParseNes(const TCHAR* rom)
{
	Allocator allocator;
	try
	{
		NesDataBase db(rom);
		Sprintf<> s;
		s.Format(_T("成功加载ROM: %s\n"), rom);
		s.Append(_T("中断向量处理程序 NMI : 0x%04X\n"), db.GetInterruptNmiAddress());
		s.Append(_T("中断向量处理程序 RESET : 0x%04X\n"), db.GetInterruptResetAddress());
		s.Append(_T("中断向量处理程序 IRQ : 0x%04X\n"), db.GetInterruptIrqAddress());
		COUT << s.ToString();

		//NesAnalyzer nesa(db);
		//nesa.Analyze();

		//for (auto sub : db.GetSubroutines())
		//{
		//	// 生成三地址码
		//	TACTranslater ntt(db, allocator);
		//	TACSubroutine* tacSub = ntt.Translate(sub);
		//	/*COUT << _T("\n三地址码:\n");
		//	tacSub->Dump();*/

		//	// 生成C代码
		//	CTranslater translater(allocator, db);
		//	auto func = translater.TranslateSubroutine(tacSub);

		//	COUT << endl << func->name.c_str() << _T(":") << endl;
		//	COUT << func->GetBody();
		//}
		//return;

		NesSubroutineParser parser(db);
		Nes::Address addr = db.GetInterruptResetAddress();
		NesSubroutine* subroutine = parser.Parse(addr);
		COUT << _T("\n基本块:\n");
		parser.Dump();

		// 生成三地址码
		TACTranslater ntt(db, allocator);
		TACSubroutine* tacSub = ntt.Translate(subroutine);
		COUT << _T("\n三地址码:\n");
		tacSub->Dump();

		// 对三地址码进行窥孔优化
		TACPeephole ph(db);
		ph.Optimize(tacSub);
		COUT << _T("\n窥孔优化后:\n");
		tacSub->Dump();

		// 对三地址码进行死代码消除
		TACDeadCodeElimination dce(db);
		dce.Optimize(tacSub);
		COUT << _T("\n死代码消除后:\n");
		tacSub->Dump();

		// 生成C代码
		/*CTranslater translater(allocator, db);
		auto func = translater.TranslateSubroutine(tacSub);
		COUT << func->GetBody();*/

		// 分析定值到达
		//ReachingDefinition rd(db, allocator);
		//rd.Analyze(tacSub);

		// 活跃变量分析
		//LiveVariableAnalysis lva(db, allocator);
		//lva.Analyze(tacSub);

	
	}
	catch (Exception& e)
	{
		COUT << e.Message() << endl;
	}
	catch (std::exception& e)
	{
		cout << e.what() << endl;
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	ParseNes(_T(R"(D:\FC\miaoliro.nes)"));
	system("pause");
	return 0;
}