#pragma once
#include "TACOptimizer.h"

// 对三地址进行窥孔优化
class TACPeephole : public TACOptimizer
{
public:
	TACPeephole(NesDataBase& db);
	~TACPeephole();

	virtual void Optimize(TACSubroutine* subroutine) override;

};

