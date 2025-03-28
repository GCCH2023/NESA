#include "stdafx.h"
#include "TACDeadCodeElimination.h"
#include "LiveVariableAnalysis.h"
#include "TACFunction.h"
#include "NesDataBase.h"

TACDeadCodeElimination::TACDeadCodeElimination(NesDataBase& db, Allocator& allocator_) :
TACOptimizer(db),
allocator(allocator_)
{

}

TACDeadCodeElimination::~TACDeadCodeElimination()
{
}

// 在集合中标记变量被使用
void MarkUse(NodeSet& varUses, TACOperand& operand)
{
	if (IsAxyNvzc(operand))
	{
		varUses += operand.GetValue();
	}
	else if (operand.IsTemp())
	{
		varUses += operand.GetValue() + TAC_ANALIZE_REG_COUNT;
	}
}

// 如果变量需要标记使用就标记
void TryMarkUse(NodeSet& varUses, TACOperand& operand)
{
	if (IsAxyNvzc(operand) || operand.IsTemp())
		MarkUse(varUses, operand);
}

// 在集合中标记变量没被使用
void MarkUnuse(NodeSet& varUses, TACOperand& operand)
{
	int index = operand.GetValue();
	if (operand.IsTemp())
		index += TAC_ANALIZE_REG_COUNT;
	varUses -= index;
}

// 在集合中标记变量被定义
void MarkDefinition(NodeSet& varDefs, TACOperand& operand)
{
	int index = operand.GetValue();
	if (operand.IsTemp())
		index += TAC_ANALIZE_REG_COUNT;
	varDefs += index;
}


// 判断变量是否被使用
bool IsUsed(NodeSet& out, NodeSet& varUses, NodeSet& varDefs, TACOperand& operand)
{
	int index = operand.GetValue();
	if (operand.IsTemp())
		index += TAC_ANALIZE_REG_COUNT;
	// 被基本块后面的指令使用
	if (varUses.Contains(index))
		return true;
	// 被新定值覆盖了
	if (varDefs.Contains(index))
		return false;
	// 在出口处活跃
	return out.Contains(index);
}

// void DumpAllBasicBlockLiveVariables(TACBasicBlockList& blocks);

// 如何一条三地址码是跳转地址，那么消除后，这个地址就没了
// 应该改为跳转到下一条代码的地址，目前没有实现
// 基本块的开始地址保留最初的地址好了
void TACDeadCodeElimination::Optimize(TACFunction* subroutine)
{
	Reset();
	this->tacFunc = subroutine;

	// 首先进行活跃变量分析
	LiveVariableAnalysis lva(db, allocator);
	lva.SetExitOut(subroutine->GetReturnFlag());
	lva.Analyze(subroutine);
	// DumpAllBasicBlockLiveVariables(subroutine->GetBasicBlocks());

	// 遍历每个基本块，消除死代码（对寄存器赋值了但没有使用到的三地址码）
	for (auto block : subroutine->GetBasicBlocks())
	{
		auto& codes = block->GetCodes();
		if (codes.empty())
			continue;
		NodeSet varUses;  // 寄存器变量 + 临时变量 是否被当前基本块当前代码后面的代码使用
		NodeSet varDefs;  // 是否遇到过了变量的定值，只有变量的最后一个定值可以到达基本块出口
		for (auto it = codes.rbegin(); it != codes.rend();)
		{
			auto tac = *it;
			// 数组需要特殊处理
			switch (tac->op)
			{
			case TACOperator::ARRAY_GET:
				break;  // 获取数组元素，不需要特殊处理
			case TACOperator::ARRAY_SET:
				// x[y] = z, z是寄存器，标记被使用
				MarkUse(varUses, tac->z);
				MarkUse(varUses, tac->x);
				MarkUse(varUses, tac->y);
				++it;
				continue;
			case TACOperator::CALL:
			{
									  auto sub = db.FindSubroutine(tac->z.GetValue());
									  if (sub)  // 找不到说明还没分析，先不管
									  {
										  if (sub->flag & SUBF_PARAM)
										  {
											  varUses |= NodeSet(sub->flag & SUBF_PARAM);  // 标记参数AXY被使用
										  }
										  auto rets = (sub->flag & SUBF_RETURN) >> 3;
										  if (rets)
										  {
											  varDefs |= NodeSet(rets);  // 标记定值
										  }
									  }
									  ++it;
									  continue;  // 函数调用不要删除
			}
			}
			if (IsAxyNvzcTemp(tac->z))
			{
				// 判断它是否被后面的基本块引用，也就是在这个基本块的出口处，这个变量是活跃的
				// 判断它是否被这个基本块后面的代码引用
				auto live = (BasicBlockLiveVariableSet*)block->tag;
				if (!IsUsed(live->out, varUses, varDefs, tac->z))
				{
					// 删除这条代码
					it = TACList::reverse_iterator(codes.erase((++it).base()));
					continue;
				}
				MarkUnuse(varUses, tac->z);  // 新的定值点，则标记前面的AXY没有被使用
				MarkDefinition(varDefs, tac->z);
			}
			// 如果这条代码使用到了某个变量，就标记使用
			TryMarkUse(varUses, tac->x);
			TryMarkUse(varUses, tac->y);
			++it;
		}
	}

	// 输出地址到基本块的映射
	//Sprintf<> s;
	//s.Format(_T("地址-基本块映射：\n"));
	//for (auto it : addrMap)
	//{
	//	s.Append(_T("%04X -> %04X\n"), it.first, it.second->GetStartAddress());
	//}
	//COUT << s.ToString();

	CorrectJumpAddress();

	// 删除没用到的临时变量
	std::vector<bool> tempUse(256);
	for (auto block : subroutine->GetBasicBlocks())
	{
		for (auto tac : block->GetCodes())
		{
			if (tac->x.IsTemp())
				tempUse[tac->x.GetValue()] = true;
			if (tac->y.IsTemp())
				tempUse[tac->y.GetValue()] = true;
			if (tac->z.IsTemp())
				tempUse[tac->z.GetValue()] = true;
		}
	}

	auto& types = subroutine->GetTempVariableTypes();
	for (size_t i = 0; i < types.size(); ++i)
	{
		if (tempUse[i] == false)
		{
			types[i] = nullptr;
		}
	}
}

void TACDeadCodeElimination::Reset()
{

}

void TACDeadCodeElimination::CorrectJumpAddress()
{
	auto tacs = tacFunc->GetCodes();
	// 修正跳转地址
	for (auto block : tacFunc->GetBasicBlocks())
	{
		auto& codes = block->GetCodes();
		if (codes.empty())
			continue;
		auto tac = *codes.rbegin();
		if (tac->IsConditionalJump())
		{
			// 本来是跳转到基本块的第一条指令的地址的，
			// 前面 n 条指令可能被删除了，现在跳转到删除指令的后面指令
			// 查找第一条地址大于原来的跳转地址的指令
			auto it = std::lower_bound(tacs.begin(), tacs.end(), tac->z.GetValue(),
				[](const TAC* tac, Nes::Address address) {
				return tac->address < address;
			});
			if (it == tacs.end())
			{
				throw Exception(_T("错误: 跳转地址丢失"));
			}
			tac->z.SetValue((*it)->address);
		}
	}
}
