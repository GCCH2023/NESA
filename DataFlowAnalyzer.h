#pragma once
class TACFunction;
class NesDataBase;
class TACBasicBlock;

// �����������Ļ���
// ����ָ�����͵Ĳ��������з���
// �ڷ�����������ָ��ʱ����Ҫ֪�����õĺ����Ƿ������˼Ĵ���
// template<typename TParam>
class DataFlowAnalyzer
{
public:
	DataFlowAnalyzer(NesDataBase& db);
	~DataFlowAnalyzer();
	// ���з���
	virtual void Analyze(TACFunction* subroutine);
protected:
	// ��ʼ��
	virtual void Initialize() = 0;
	// ���������飬�����Ƿ�������������л���������Ž���
	virtual bool IteraterBasicBlock(TACBasicBlock* block) = 0;
	// ����ʼ��
	virtual void Uninitialize();

	NesDataBase& db;
	TACFunction* subroutine;
};

