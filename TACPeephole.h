#pragma once
#include "TACOptimizer.h"

// ������ַ���п����Ż�
class TACPeephole : public TACOptimizer
{
public:
	TACPeephole(NesDataBase& db);
	~TACPeephole();

	virtual void Optimize(TACSubroutine* subroutine) override;
	// �����㷨�õ�������
	void Reset();
};

