#pragma once
#include "TACOptimizer.h"
class TACFunction;

// 进行死代码消除
class TACDeadCodeElimination :
	public TACOptimizer
{
public:
	// allocator 用于分配临时对象
	TACDeadCodeElimination(NesDataBase& db, Allocator& allocator);
	~TACDeadCodeElimination();

	virtual void Optimize(TACFunction* subroutine) override;

	virtual void Reset() override;
protected:
	// 修正跳转地址
	void CorrectJumpAddress();
protected:
	Allocator& allocator;
	TACFunction* tacFunc;
};

