#include "stdafx.h"
#include "TACFlagRegisterOptimizer.h"



TACFlagRegisterOptimizer::TACFlagRegisterOptimizer(Allocator& allocator_):
allocator(allocator_)
{

}

TACFlagRegisterOptimizer::~TACFlagRegisterOptimizer()
{
}

std::vector<TAC*>& TACFlagRegisterOptimizer::Optimize(std::vector<TAC*>& codes)
{
	// 如果一个标志位被赋值了，但是没有使用就重新赋值，那么删除它
	size_t definition[10];  // 标志寄存器的定值三地址码索引
	std::fill(definition, definition + 10, INT_MAX);
	std::vector<bool> used(codes.size());  // 该条三地址码是否被使用
	std::fill(used.begin(), used.end(), true);

	for (size_t i = 0; i < codes.size(); ++i)
	{
		auto tac = codes[i];
		if (tac->z.IsRegister())
		{
			int value = tac->z.GetValue();
			if (value >= TAC_REG_N && value <= TAC_REG_C)
			{
				definition[value] = i;  // 没有定值，则标记定值点
				used[i] = false;  // 标记这条指令没有被使用
			}
		}
		if (tac->x.IsRegister())
		{
			int value = tac->x.GetValue();
			if (value >= TAC_REG_N && value <= TAC_REG_C)
			{
				used[i] = true;  // 标记这条指令被使用
			}
		}
		if (tac->y.IsRegister())
		{
			int value = tac->y.GetValue();
			if (value >= TAC_REG_N && value <= TAC_REG_C)
			{
				used[i] = true;  // 标记这条没有被使用
			}
		}
	}

	// 最后的定值点可能被其他基本块使用，全部标记为使用
	for (size_t i = TAC_REG_N; i <= TAC_REG_C; ++i)
	{
		if (definition[i] != INT_MAX)
			used[definition[i]] = true;
	}

	for (size_t i = 0; i < codes.size(); ++i)
	{
		if (used[i])
		{
			result.push_back(codes[i]);
		}
	}

	return result;
}
