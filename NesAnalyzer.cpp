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
	size_t index;  // ���б��е�����
	NodeSet calls;  // ���õĺ�������Ҫ�ȷ���
};

NesAnalyzer::NesAnalyzer(NesDataBase& db_):
db(db_)
{

}

NesAnalyzer::~NesAnalyzer()
{
}


// �����ӳ���
NesSubroutine* NesAnalyzer::AnalyzeSubroutine(NesSubroutineParser& parser, Nes::Address addr)
{
	parser.Reset();
	NesSubroutine* subroutine = parser.Parse(addr);

	AddSubroutine(subroutine);
	// ������һ���ӳ���󣬽�����Ҫ���������õ��ӳ���
	//printf("�ӳ��� %04X ���ã�", addr);
	for (auto addr : subroutine->GetCalls())
	{
		//printf("%04X, ", addr);
		if (IsSubroutineAnalyzed(addr))
			continue;

		AnalyzeSubroutine(parser, addr);
	}
	//printf("\n");
	//printf("�����ӳ��� %04X - %04X\n", subroutine->GetStartAddress(), subroutine->GetEndAddress());
	return subroutine;
}

void NesAnalyzer::DumpCallRelation(NesSubroutine* subroutine)
{
	TCHAR buffer[256];
	TCHAR* p = buffer;
	p += _stprintf_s(buffer, 256, _T("�ӳ��� %04X - %04X ���ã�"), subroutine->GetStartAddress(), subroutine->GetEndAddress());
	auto& calls = subroutine->GetCalls();
	if (calls.empty())
		_stprintf_s(p, 256 - (p - buffer), _T("��\n"));
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
	// ���ȸ������ӳ�����
	auto& subroutines = db.GetSubroutines();
	if (subroutines.size() > MAX_NODE)
	{
		Sprintf<> s;
		s.Format(_T("λ���޷���ʾ %d �����ϵ��ӳ���"), MAX_NODE);
		throw Exception(s.ToString());  // ��Ҫ�Զ�������ʵ��
	}

	size_t index = 0;
	// ���������������ڷ���
	for (auto sub : subroutines)
	{
		auto sd = allocator.New<SubroutineData>();
		sd->index = index++;
		sub->tag = sd;
	}
	// ��ʼ���ӳ�����ü�
	for (auto sub : subroutines)
	{
		SubroutineData* sd = (SubroutineData*)sub->tag;
		for (auto addr : sub->GetCalls())
		{
			auto callSub = db.FindSubroutine(addr);
			sd->calls |= 1 << ((SubroutineData*)callSub->tag)->index;  // �����ӳ���ĵ��ü�
		}
	}
	// �������������ӳ���
	NodeSet analyzeSubs = 0;  // �Ѿ��������˵��ӳ���
	Allocator tempAllocator;
	TACTranslater tacTranslater(db, tempAllocator);
	int iter = 0;
	while (true)
	{
		NodeSet oldState = analyzeSubs;
		//printf("�������� %d\n", iter++);
		// ����ÿ���ӳ��򣬷�������������
		for (auto sub : subroutines)
		{
			auto sd = (SubroutineData*)sub->tag;
			if ((analyzeSubs & (1 << sd->index)) == 0 && (sd->calls & analyzeSubs) == sd->calls)
			{
				// û�з��������������õ��ӳ��򶼷������ˣ���ô���Է�������ӳ�����
				auto tacSub = tacTranslater.Translate(sub);
				AnalyzeTACSubroutine(tacSub);
				sub->flag = tacSub->flag;
				analyzeSubs |= 1 << sd->index;  // ��Ǵ��ӳ����Ѿ�����
			}
		}
		if (analyzeSubs == oldState)
		{
			// �ж��Ƿ�ȫ��������ϣ�Ҳ�����Ǵ��ڻ�״���õ��·����޷�������ȥ
			NodeSet mask = (1 << subroutines.size()) - 1;
			if (analyzeSubs != mask)
			{
				throw Exception(_T("����ʧ�ܣ������ӳ�����ù�ϵʱ������״����"));
			}
			break;
		}
	}
}

void NesAnalyzer::AnalyzeTACSubroutine(TACSubroutine* subroutine)
{
	auto& blocks = subroutine->GetBasicBlocks();
	// ���л�Ծ������������ȷ���Ƿ�ʹ��AXY��Ϊ����
	LiveVariableAnalysis lva(db, allocator);
	lva.Analyze(subroutine);
	// �����ڻ�������ʹ����AXY����AXY��Ϊ����
	auto entry = *blocks.begin();
	auto lives = (BasicBlockLiveVariableSet*)entry->tag;
	subroutine->flag = (uint32_t)lives->in.ToInteger();

	// ���е��ﶨֵ���������AXY�ܹ����ﷵ�ػ�����飬������Ƿ���ֵ
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

// ��Ҫ�ǽ��� NES �е������жϴ�����򣬸������ǵ��õ��ӳ����ַ��
// �𽥽��������е��ӳ������
void NesAnalyzer::Analyze()
{
	NesSubroutineParser parser(db);

	Nes::Address addr = db.GetInterruptResetAddress();

	AnalyzeSubroutine(parser, addr);

	DumpAllCallRelation();

	// �������ӳ���ķ�Χ�󣬷����ӳ���� AXY �Ĵ�����ʹ�����
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
