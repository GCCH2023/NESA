#pragma once
class TACFunction;
class NesDataBase;

// �Ż�����ַ��
class TACOptimizer
{
public:
	TACOptimizer(NesDataBase& db);
	~TACOptimizer();

	virtual void Optimize(TACFunction* subroutine);

protected:
	NesDataBase& db;
};

