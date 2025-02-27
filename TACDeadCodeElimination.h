#pragma once
#include "TACOptimizer.h"

// ��������������
class TACDeadCodeElimination :
	public TACOptimizer
{
public:
	TACDeadCodeElimination(NesDataBase& db);
	~TACDeadCodeElimination();

	virtual void Optimize(TACSubroutine* subroutine) override;

	void Reset();
};

