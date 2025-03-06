#pragma once
class TACFunction;
class NesDataBase;

// 优化三地址码
class TACOptimizer
{
public:
	TACOptimizer(NesDataBase& db);
	~TACOptimizer();

	virtual void Optimize(TACFunction* subroutine);

protected:
	NesDataBase& db;
};

