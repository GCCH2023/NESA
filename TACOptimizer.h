#pragma once
class TACSubroutine;
class NesDataBase;

// 优化三地址码
class TACOptimizer
{
public:
	TACOptimizer(NesDataBase& db);
	~TACOptimizer();

	virtual void Optimize(TACSubroutine* subroutine);

protected:
	NesDataBase& db;
};

