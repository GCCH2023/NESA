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

// 用DAG生成基本块的C语句测试
// a = b + c
// b = a - d
// c = b + c
// d = a - d
#include "BaiscBlockDAG.h"
void BaiscBlockDAGTest()
{
	// 1. 构造三地址码
	TACOperand a(TACOperand::TEMP | 0);
	TACOperand b(TACOperand::TEMP | 1);
	TACOperand c(TACOperand::TEMP | 2);
	TACOperand d(TACOperand::TEMP | 3);

	TACBasicBlock block;
	TAC code0(TACOperator::ADD, a, b, c);
	TAC code1(TACOperator::SUB, b, a, d);
	TAC code2(TACOperator::ADD, c, b, c);
	TAC code3(TACOperator::SUB, d, a, d);
	block.AddTAC(&code0);
	block.AddTAC(&code1);
	block.AddTAC(&code2);
	block.AddTAC(&code3);

	BaiscBlockDAG bbDag(GetCDB().GetAllocator());
	auto statement = bbDag.Translate(&block);
	COUT << statement;
}

// 全局变量测试
void GlobalTest()
{
	Sprintf<> s;
	for (auto g : GetCDB().GetGlobalList())
	{
		s.Format(_T("%08X\t%s\t%s\n"), g->address, ToString(g->type->GetKind()), g->name->str);
		COUT << s.ToString();
		s.Clear();
	}
}

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

		TACTranslater ntt(db, allocator);
		TACPeephole tacPh(db);
		TACDeadCodeElimination tacDce(db);
		CTranslater translater(allocator, db);
		CTreeOptimizer ctreeOptimizer;

		// 一. 从这里开始，到 return 之间的代码是从指定函数开始
		// 分析它及它调用的所有函数
		NesAnalyzer nesa(db);
		nesa.Analyze();

		// 生成C代码
		for (auto sub : db.GetSubroutines())
		{
			// 生成三地址码
			TACFunction* tacSub = ntt.Translate(sub);
			if (sub->GetStartAddress() == 0x8220)
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

			// 5. 添加到数据库
			GetCDB().AddFunction(func);
		}

		// 输出C代码
		Dump(GetCDB());
		return;

		// 二. 详细分析一个函数（不包括它调用的函数） 
		NesSubroutineParser parser(db);
		Nes::Address addr = 0x8EE6;// db.GetInterruptResetAddress();
		NesSubroutine* subroutine = parser.Parse(addr);
		COUT << _T("\n基本块:\n");
		// 输出 FC 指令
		parser.Dump();

		// 生成三地址码
		TACFunction* tacSub = ntt.Translate(subroutine);
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
	Type* a = GetTypeManager().NewArray(TypeManager::Int, 5);
	Type* b = GetTypeManager().NewArray(TypeManager::Int, 5);
	COUT << boolalpha << (a == b) << endl;
}

int _tmain(int argc, _TCHAR* argv[])
{
	ParseNes(_T(R"(D:\FC\miaoliro.nes)"));
	// TypeTest();
	// BaiscBlockDAGTest();
	// GlobalTest();
	system("pause");
	return 0;
}