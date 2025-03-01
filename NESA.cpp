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
#include "CTreeOptimizer.h"
#include "Dump.h"
#include "CDataBase.h"

void ParseNes(const TCHAR* rom)
{
	COUT << sizeof(Type) << endl;
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

		CDataBase cdb(allocator);
		TACTranslater ntt(db, allocator);
		TACPeephole tacPh(db);
		TACDeadCodeElimination tacDce(db);
		CTranslater translater(allocator, db, cdb);
		CTreeOptimizer ctreeOptimizer;

		NesAnalyzer nesa(db);
		nesa.Analyze();

		for (auto sub : db.GetSubroutines())
		{
			// 生成三地址码
			TACSubroutine* tacSub = ntt.Translate(sub);
			/*COUT << _T("\n三地址码:\n");
			tacSub->Dump();*/

			// 1. 进行窥孔优化
			tacPh.Optimize(tacSub);

			// 2. 进行死代码消除
			tacDce.Optimize(tacSub);

			// 3. 生成C代码
			auto func = translater.TranslateSubroutine(tacSub);

			// 4. 优化C代码
			ctreeOptimizer.Optimize(func->GetBody());

			// 5. 输出函数代码
			COUT << endl;
			DumpDefinition(func);
		}
		return;

		NesSubroutineParser parser(db);
		Nes::Address addr = 0x90CC; // db.GetInterruptResetAddress();
		NesSubroutine* subroutine = parser.Parse(addr);
		COUT << _T("\n基本块:\n");
		parser.Dump();

		// 生成三地址码
		TACSubroutine* tacSub = ntt.Translate(subroutine);
		COUT << _T("\n三地址码:\n");
		tacSub->Dump();

		// 对三地址码进行窥孔优化
		tacPh.Optimize(tacSub);
		COUT << _T("\n窥孔优化后:\n");
		tacSub->Dump();

		// 对三地址码进行死代码消除
		tacDce.Optimize(tacSub);
		COUT << _T("\n死代码消除后:\n");
		tacSub->Dump();

		// 生成C代码
		auto func = translater.TranslateSubroutine(tacSub);
		//COUT << func->GetBody();

		//COUT << _T("\n语法树结构:\n");
		//DumpCNodeStructures(COUT, func->GetBody(), 0);

		// 优化C代码结构
		ctreeOptimizer.Optimize(func->GetBody());
		//COUT << _T("\n优化语法树结构后:\n");
		//DumpCNodeStructures(COUT, func->GetBody(), 0);
		COUT << endl << func->GetBody();

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

void TypeTest()
{
	Allocator allocator;
	TypeManager tm(allocator);
	Type* a = tm.NewArray(tm.Int, 5);
	Type* b = tm.NewArray(tm.Int, 5);
	COUT << boolalpha << (a == b) << endl;
}

int _tmain(int argc, _TCHAR* argv[])
{
	// ParseNes(_T(R"(D:\FC\移动.nes)"));
	ParseNes(_T(R"(D:\FC\miaoliro.nes)"));
	// TypeTest();
	system("pause");
	return 0;
}