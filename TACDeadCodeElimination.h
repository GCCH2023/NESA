#pragma once
#include "TACOptimizer.h"

// 进行死代码消除
class TACDeadCodeElimination :
	public TACOptimizer
{
public:
	TACDeadCodeElimination(NesDataBase& db);
	~TACDeadCodeElimination();

	virtual void Optimize(TACFunction* subroutine) override;

	virtual void Reset() override;
};

