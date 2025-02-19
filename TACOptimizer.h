#pragma once
class TACSubroutine;
class NesDataBase;

// �Ż�����ַ��
class TACOptimizer
{
public:
	TACOptimizer(NesDataBase& db);
	~TACOptimizer();

	virtual void Optimize(TACSubroutine* subroutine);

protected:
	NesDataBase& db;
};

