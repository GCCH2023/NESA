#include "stdafx.h"
#include "TACDeadCodeElimination.h"
#include "LiveVariableAnalysis.h"
#include "TAC.h"

TACDeadCodeElimination::TACDeadCodeElimination(NesDataBase& db) :
TACOptimizer(db)
{

}

TACDeadCodeElimination::~TACDeadCodeElimination()
{
}

// ���һ������ַ������ת��ַ����ô�����������ַ��û��
// Ӧ�ø�Ϊ��ת����һ������ĵ�ַ��Ŀǰû��ʵ��
// ������Ŀ�ʼ��ַ��������ĵ�ַ����
void TACDeadCodeElimination::Optimize(TACSubroutine* subroutine)
{
	Reset();

	Allocator allocator(4 * 1024 * 1024);
	// ���Ƚ��л�Ծ��������
	LiveVariableAnalysis lva(db, allocator);
	lva.Analyze(subroutine);

	std::unordered_map<uint32_t, TACBasicBlock*> addrMap;  // �������һ��ָ��ĵ�ַ���������ӳ��

	// ����ÿ�������飬���������루�ԼĴ�����ֵ�˵�û��ʹ�õ�������ַ�룩
	for (auto block : subroutine->GetBasicBlocks())
	{
		auto& codes = block->GetCodes();
		addrMap[codes[0]->address] = block;
		NodeSet axyUses;  // AXY �Ƿ񱻵�ǰ�����鵱ǰ�������Ĵ���ʹ��
		for (auto it = codes.rbegin(); it != codes.rend();)
		{
			auto tac = *it;
			if (tac->z.IsRegister() && tac->z.GetValue() <= Nes::NesRegisters::Y)
			{
				// �ж����Ƿ񱻺���Ļ��������ã�Ҳ���������������ĳ��ڴ�����������ǻ�Ծ��
				// �ж����Ƿ�������������Ĵ�������
				auto live = (BasicBlockLiveVariableSet*)block->tag;
				if (!live->out.Contains(tac->z.GetValue()) && !axyUses.Contains(tac->z.GetValue()))
				{
					// ɾ����������
					it = TACList::reverse_iterator(codes.erase((++it).base()));
					continue;
				}
				axyUses -= tac->z.GetValue();  // ��AXY�Ķ�ֵ������ǰ���AXYû�б�ʹ��
			}
			// �����������ʹ�õ���AXY���ͱ��
			if (tac->x.IsRegister() && tac->x.GetValue() <= Nes::NesRegisters::Y)
			{
				axyUses += tac->x.GetValue();
			}
			if (tac->y.IsRegister() && tac->y.GetValue() <= Nes::NesRegisters::Y)
			{
				axyUses += tac->y.GetValue();
			}
			++it;
		}
	}

	// ������ת��ַ
	for (auto block : subroutine->GetBasicBlocks())
	{
		auto& codes = block->GetCodes();
		if (codes.empty())
			continue;
		auto tac = *codes.rbegin();
		if (tac->IsConditionalJump())
		{
			// ��������ת��������ĵ�һ��ָ��ĵ�ַ�ģ�
			// ǰ�� n ��ָ����ܱ�ɾ���ˣ�
			// ������ת��ɾ����Ļ�����ĵ�һ��ָ��ĵ�ַ
			auto b = addrMap[tac->z.GetValue()];
			uint32_t newAddr = b->GetCodes()[0]->address;
			tac->z.SetValue(newAddr);
		}
	}
}

void TACDeadCodeElimination::Reset()
{

}
