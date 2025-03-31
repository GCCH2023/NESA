// NESA.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "NesDataBase.h"
using namespace std;
using namespace Nes;

#include "NesSubroutineParser.h"
#include "TACTranslater.h"
#include "TACTranslater1.h"
#include "CGraphTranslator.h"
#include "CDirectTranslator.h"
#include "CCommonTranslator.h"

#include "ReachingDefinition.h"
#include "LiveVariableAnalysis.h"
#include "NesAnalyzer.h"
#include "TACPeephole.h"
#include "TACDeadCodeElimination.h"
#include "CTreeOptimizer.h"
#include "Dump.h"
#include "CDataBase.h"
#include "GlobalParser.h"
#include "TACFunctionParser.h"
#include "SubroutineRangeParser.h"

// 全局变量测试
void GlobalTest()
{
	Sprintf<> s;
	// 输出全局变量列表
	for (auto g : GetCDB().GetGlobalList())
	{
		s.Format(_T("%08X\t%s\t%s\n"), g->address, ToString(g->type->GetKind()), g->name->str);
		COUT << s.ToString();
		s.Clear();
	}
	// 查找全局变量
	auto v = GetCDB().GetGlobalVariable(0x2000);
	if (v)
	{
		COUT << _T("找到全局变量 ") << v->name->str << endl;
	}
	else
	{
		COUT << _T("找不到全局变量 ") << endl;
	}
	v = GetCDB().GetGlobalVariable(0x2001);
	if (v)
	{
		COUT << _T("找到全局变量 ") << v->name->str << endl;
	}
	else
	{
		COUT << _T("找不到全局变量 ") << endl;
	}

	GetCDB().AddGlobalVariable(10, GetCDB().GetAXYType());
	v = GetCDB().GetGlobalVariable(12);
	if (v)
	{
		COUT << _T("找到全局变量 ") << v->name->str << endl;
	}
	else
	{
		COUT << _T("找不到全局变量 ") << endl;
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

		TACTranslater1 ntt(db, allocator);
		TACPeephole tacPh(db);
		TACDeadCodeElimination tacDce(db, allocator);
		CCommonTranslator translater(allocator);
		CTreeOptimizer ctreeOptimizer;
		GlobalParser globalParser(db);

		// 一. 从这里开始，到 return 之间的代码是从指定函数开始
		// 分析它及它调用的所有函数
		NesAnalyzer nesa(db);
		nesa.Analyze();

		// 生成C代码
		for (auto sub : db.GetSubroutines())
		{
			// 生成三地址码
			TACFunction* tacSub = ntt.Translate(sub);
			/*if (sub->GetStartAddress() == 0x8220)
				COUT << _T("\n三地址码:\n");*/
			// tacSub->Dump();

			// 1. 进行窥孔优化
			tacPh.Optimize(tacSub);

			// 2. 进行死代码消除
			tacDce.Optimize(tacSub);

			// 3. 生成C代码
			auto func = translater.Translate(tacSub);

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
		Nes::Address addr = 0xF8D9; // db.GetInterruptNmiAddress();

		NesSubroutine* subroutine = parser.Parse(addr);
		COUT << _T("\n基本块:\n");
		// 输出 FC 指令
		parser.Dump();

		// 解析全局变量
		globalParser.Parse(subroutine);

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
		auto func = translater.Translate(tacSub);
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

#include "TACBasicBlockOptimizer.h"
// 三地址码优化测试
void TACBasicBlockOptimizerTest()
{
	Allocator allocator;
	TACBasicBlockOptimizer opt(allocator);
	/*
	1. t0 = 5
	2. t1 = 10
	3. t2 = t0 + t1       // t2 = 15 常量替换，常量折叠
	4. t3 = t3 + t0           // 死代码消除
	5. t3 = t4 - 10       // t3 = t4 - 10
	6. t4 = t4 - 10           // t4 = t3, 公共子表达式消除
	*/
	//temp2 = 0x0A + 0x05
	//	temp3 = temp4 - 0x0A
	std::vector<TAC*> codes(32);
	codes.clear();
	TACOperand t0(TACOperand::TEMP | 0);
	TACOperand t1(TACOperand::TEMP | 1);
	TACOperand t2(TACOperand::TEMP | 2);
	TACOperand t3(TACOperand::TEMP | 3);
	TACOperand t4(TACOperand::TEMP | 4);
	codes.push_back(allocator.New<TAC>(TACOperator::ASSIGN, t0, 5));
	codes.push_back(allocator.New<TAC>(TACOperator::ASSIGN, t1, 10));
	codes.push_back(allocator.New<TAC>(TACOperator::ADD, t2, t1, t0));
	codes.push_back(allocator.New<TAC>(TACOperator::ADD, t3, t3, t0));
	codes.push_back(allocator.New<TAC>(TACOperator::SUB, t3, t4, 10));
	codes.push_back(allocator.New<TAC>(TACOperator::SUB, t4, t4, 10));

	auto& result = opt.Optimize(codes);

	for (auto tac : result)
	{
		COUT << tac << std::endl;
	}
}

// 三地址码函数分析
void TACFunctionParserTest(const TCHAR* rom, Nes::Address address = 0)
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

		TACTranslater1 ntt(db, allocator);
		// CGraphTranslator translater(allocator);
		// CDirectTranslator translater(allocator);
		CCommonTranslator translater(allocator);
		CTreeOptimizer ctreeOptimizer;
		GlobalParser globalParser(db);
		TACFunctionParser funcParser(db);

		// 二. 详细分析一个函数（不包括它调用的函数） 
		NesSubroutineParser parser(db);
		if (address == 0)
			address = db.GetInterruptNmiAddress();
		Nes::Address addr = address;

		NesSubroutine* subroutine = parser.Parse(addr);
		COUT << _T("\n基本块:\n");
		// 输出 FC 指令
		parser.Dump();

		// 解析全局变量
		globalParser.Parse(subroutine);

		// 生成三地址码
		TACFunction* tacSub = ntt.Translate(subroutine);
		COUT << _T("\n三地址码:\n");
		tacSub->Dump();

		// 分析函数
		funcParser.Parse(tacSub);
		COUT << _T("\n分析函数后:\n");
		tacSub->Dump();

		// 生成C代码
		auto func = translater.Translate(tacSub);
		//COUT << func->GetBody();

		//COUT << _T("\n语法树结构:\n");
		//DumpCNodeStructures(COUT, func->GetBody(), 0);

		// 优化C代码结构
		//ctreeOptimizer.Optimize(func->GetBody());
		//COUT << _T("\n优化语法树结构后:\n");
		// DumpCNodeStructures(COUT, func->GetBody(), 0);
		COUT << endl;
		DumpDefinition(func);
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

// 保存ROM的代码到 .prg 文件
void SavePRG(const TCHAR* rom)
{
	Cartridge cartridge;
	cartridge.LoadRom(rom);

	FILE* fp;
	Sprintf<> s;
	s.Format(_T("%s.prg"), rom);
	_tfopen_s(&fp, s.ToString(), _T("wb"));
	fwrite(cartridge.RawGetData(16), cartridge.GetPRGCount() * 16 * 1024, 1, fp);
	fclose(fp);
	COUT << _T("写入文件 ") << s.ToString() << _T(" 成功\n");
}

void SubroutineRangeParserTest(const TCHAR* rom)
{
	NesDataBase db(rom);
	SubroutineRangeParser srp(db);

	srp.Parse();

}

int _tmain(int argc, _TCHAR* argv[])
{
	const TCHAR* rom = _T(R"(D:\FC\miaoliro.nes)");
	
	// GetTypeManager();  // 初始化

	//SavePRG(rom);

	// ParseNes(rom);
	// TypeTest();
	// BaiscBlockDAGTest();
	// GlobalTest();
	// TACBasicBlockOptimizerTest();
	// TACFunctionParserTest(rom, 0x8E19);
	SubroutineRangeParserTest(rom);
	system("pause");
	return 0;
}