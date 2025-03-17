#pragma once
#include "TAC.h"

// 去除基本块TAC中的标志位
class TACFlagRegisterOptimizer
{
public:
	// 用于分配优化结果
	TACFlagRegisterOptimizer(Allocator& allocator);
	~TACFlagRegisterOptimizer();

	std::vector<TAC*>& Optimize(std::vector<TAC*>& codes);
protected:
	Allocator& allocator;
	std::vector<TAC*> result;
};

