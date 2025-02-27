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

// 如何一条三地址码是跳转地址，那么消除后，这个地址就没了
// 应该改为跳转到下一条代码的地址，目前没有实现
// 基本块的开始地址保留最初的地址好了
void TACDeadCodeElimination::Optimize(TACSubroutine* subroutine)
{
	Reset();

	Allocator allocator(4 * 1024 * 1024);
	// 首先进行活跃变量分析
	LiveVariableAnalysis lva(db, allocator);
	lva.Analyze(subroutine);

	std::unordered_map<uint32_t, TACBasicBlock*> addrMap;  // 基本块第一条指令的地址到基本块的映射

	// 遍历每个基本块，消除死代码（对寄存器赋值了但没有使用到的三地址码）
	for (auto block : subroutine->GetBasicBlocks())
	{
		auto& codes = block->GetCodes();
		addrMap[codes[0]->address] = block;
		NodeSet axyUses;  // AXY 是否被当前基本块当前代码后面的代码使用
		for (auto it = codes.rbegin(); it != codes.rend();)
		{
			auto tac = *it;
			if (tac->z.IsRegister() && tac->z.GetValue() <= Nes::NesRegisters::Y)
			{
				// 判断它是否被后面的基本块引用，也就是在这个基本块的出口处，这个变量是活跃的
				// 判断它是否被这个基本块后面的代码引用
				auto live = (BasicBlockLiveVariableSet*)block->tag;
				if (!live->out.Contains(tac->z.GetValue()) && !axyUses.Contains(tac->z.GetValue()))
				{
					// 删除这条代码
					it = TACList::reverse_iterator(codes.erase((++it).base()));
					continue;
				}
				axyUses -= tac->z.GetValue();  // 对AXY的定值，则标记前面的AXY没有被使用
			}
			// 如果这条代码使用到了AXY，就标记
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

	// 修正跳转地址
	for (auto block : subroutine->GetBasicBlocks())
	{
		auto& codes = block->GetCodes();
		if (codes.empty())
			continue;
		auto tac = *codes.rbegin();
		if (tac->IsConditionalJump())
		{
			// 本来是跳转到基本块的第一条指令的地址的，
			// 前面 n 条指令可能被删除了，
			// 现在跳转到删除后的基本块的第一条指令的地址
			auto b = addrMap[tac->z.GetValue()];
			uint32_t newAddr = b->GetCodes()[0]->address;
			tac->z.SetValue(newAddr);
		}
	}
}

void TACDeadCodeElimination::Reset()
{

}
