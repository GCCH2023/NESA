// NESA.cpp : �������̨Ӧ�ó������ڵ㡣
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

// ��DAG���ɻ������C������
// a = b + c
// b = a - d
// c = b + c
// d = a - d
#include "BaiscBlockDAG.h"
void BaiscBlockDAGTest()
{
	// 1. ��������ַ��
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

// ȫ�ֱ�������
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
		s.Format(_T("�ɹ�����ROM: %s\n"), rom);
		s.Append(_T("�ж������������ NMI : 0x%04X\n"), db.GetInterruptNmiAddress());
		s.Append(_T("�ж������������ RESET : 0x%04X\n"), db.GetInterruptResetAddress());
		s.Append(_T("�ж������������ IRQ : 0x%04X\n"), db.GetInterruptIrqAddress());
		COUT << s.ToString();

		TACTranslater ntt(db, allocator);
		TACPeephole tacPh(db);
		TACDeadCodeElimination tacDce(db);
		CTranslater translater(allocator, db);
		CTreeOptimizer ctreeOptimizer;

		// һ. �����￪ʼ���� return ֮��Ĵ����Ǵ�ָ��������ʼ
		// �������������õ����к���
		NesAnalyzer nesa(db);
		nesa.Analyze();

		// ����C����
		for (auto sub : db.GetSubroutines())
		{
			// ��������ַ��
			TACFunction* tacSub = ntt.Translate(sub);
			if (sub->GetStartAddress() == 0x8220)
				/*COUT << _T("\n����ַ��:\n");
			tacSub->Dump();*/

			// 1. ���п����Ż�
			tacPh.Optimize(tacSub);

			// 2. ��������������
			tacDce.Optimize(tacSub);

			// 3. ����C����
			auto func = translater.TranslateSubroutine(tacSub);

			// 4. �Ż�C����
			ctreeOptimizer.Optimize(func->GetBody());

			// 5. ��ӵ����ݿ�
			GetCDB().AddFunction(func);
		}

		// ���C����
		Dump(GetCDB());
		return;

		// ��. ��ϸ����һ�������������������õĺ����� 
		NesSubroutineParser parser(db);
		Nes::Address addr = 0x8EE6;// db.GetInterruptResetAddress();
		NesSubroutine* subroutine = parser.Parse(addr);
		COUT << _T("\n������:\n");
		// ��� FC ָ��
		parser.Dump();

		// ��������ַ��
		TACFunction* tacSub = ntt.Translate(subroutine);
		COUT << _T("\n����ַ��:\n");
		tacSub->Dump();

		// ������ַ����п����Ż�
		tacPh.Optimize(tacSub);
		COUT << _T("\n�����Ż���:\n");
		tacSub->Dump();

		// ������ַ���������������
		tacDce.Optimize(tacSub);
		COUT << _T("\n������������:\n");
		tacSub->Dump();

		// ����C����
		auto func = translater.TranslateSubroutine(tacSub);
		//COUT << func->GetBody();

		//COUT << _T("\n�﷨���ṹ:\n");
		//DumpCNodeStructures(COUT, func->GetBody(), 0);

		// �Ż�C����ṹ
		ctreeOptimizer.Optimize(func->GetBody());
		//COUT << _T("\n�Ż��﷨���ṹ��:\n");
		//DumpCNodeStructures(COUT, func->GetBody(), 0);
		COUT << endl << func->GetBody();

		// ������ֵ����
		//ReachingDefinition rd(db, allocator);
		//rd.Analyze(tacSub);

		// ��Ծ��������
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