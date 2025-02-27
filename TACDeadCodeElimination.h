#pragma once
#include "TACOptimizer.h"

// 进行死代码消除
class TACDeadCodeElimination :
	public TACOptimizer
{
public:
	TACDeadCodeElimination(NesDataBase& db);
	~TACDeadCodeElimination();

	virtual void Optimize(TACSubroutine* subroutine) override;

	void Reset();
};

