#pragma once
#include "TACOptimizer.h"

// ������ַ���п����Ż�
class TACPeephole : public TACOptimizer
{
public:
	TACPeephole(NesDataBase& db);
	~TACPeephole();

	virtual void Optimize(TACFunction* subroutine) override;
	// �����㷨�õ�������
	void Reset();
};

