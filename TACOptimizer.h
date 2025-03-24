#pragma once
#include "TACFunction.h"

class NesDataBase;

// 优化三地址码
class TACOptimizer
{
public:
	TACOptimizer(NesDataBase& db);
	~TACOptimizer();
	// 执行优化
	virtual void Optimize(TACFunction* subroutine);
	// 重置优化器
	virtual void Reset();
protected:
	NesDataBase& db;
};