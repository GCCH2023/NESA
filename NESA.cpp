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

void ParseNes(const TCHAR* rom)
{
	COUT << sizeof(Type) << endl;
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
			// ��������ַ��
			TACSubroutine* tacSub = ntt.Translate(sub);
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

			// 5. �����������
			COUT << endl;
			DumpDefinition(func);
		}
		return;

		NesSubroutineParser parser(db);
		Nes::Address addr = 0x90CC; // db.GetInterruptResetAddress();
		NesSubroutine* subroutine = parser.Parse(addr);
		COUT << _T("\n������:\n");
		parser.Dump();

		// ��������ַ��
		TACSubroutine* tacSub = ntt.Translate(subroutine);
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
	TypeManager tm(allocator);
	Type* a = tm.NewArray(tm.Int, 5);
	Type* b = tm.NewArray(tm.Int, 5);
	COUT << boolalpha << (a == b) << endl;
}

int _tmain(int argc, _TCHAR* argv[])
{
	// ParseNes(_T(R"(D:\FC\�ƶ�.nes)"));
	ParseNes(_T(R"(D:\FC\miaoliro.nes)"));
	// TypeTest();
	system("pause");
	return 0;
}