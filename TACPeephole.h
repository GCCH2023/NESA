#pragma once
#include "TACOptimizer.h"

// 对三地址进行窥孔优化
class TACPeephole : public TACOptimizer
{
public:
	TACPeephole(NesDataBase& db);
	~TACPeephole();

	virtual void Optimize(TACFunction* subroutine) override;
	// 重置算法用到的数据
	void Reset();
};

