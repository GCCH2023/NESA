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

		//NesAnalyzer nesa(db);
		//nesa.Analyze();

		//for (auto sub : db.GetSubroutines())
		//{
		//	// ��������ַ��
		//	TACTranslater ntt(db, allocator);
		//	TACSubroutine* tacSub = ntt.Translate(sub);
		//	/*COUT << _T("\n����ַ��:\n");
		//	tacSub->Dump();*/

		//	// ����C����
		//	CTranslater translater(allocator, db);
		//	auto func = translater.TranslateSubroutine(tacSub);

		//	COUT << endl << func->name.c_str() << _T(":") << endl;
		//	COUT << func->GetBody();
		//}
		//return;

		NesSubroutineParser parser(db);
		Nes::Address addr = db.GetInterruptResetAddress();
		NesSubroutine* subroutine = parser.Parse(addr);
		COUT << _T("\n������:\n");
		parser.Dump();

		// ��������ַ��
		TACTranslater ntt(db, allocator);
		TACSubroutine* tacSub = ntt.Translate(subroutine);
		COUT << _T("\n����ַ��:\n");
		tacSub->Dump();

		// ������ַ����п����Ż�
		TACPeephole ph(db);
		ph.Optimize(tacSub);
		COUT << _T("\n�����Ż���:\n");
		tacSub->Dump();

		// ������ַ���������������
		TACDeadCodeElimination dce(db);
		dce.Optimize(tacSub);
		COUT << _T("\n������������:\n");
		tacSub->Dump();

		// ����C����
		/*CTranslater translater(allocator, db);
		auto func = translater.TranslateSubroutine(tacSub);
		COUT << func->GetBody();*/

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

int _tmain(int argc, _TCHAR* argv[])
{
	ParseNes(_T(R"(D:\FC\miaoliro.nes)"));
	system("pause");
	return 0;
}