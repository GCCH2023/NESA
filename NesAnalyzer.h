#pragma once
class NesDataBase;
class NesSubroutine;
class NesSubroutineParser;
class TACSubroutine;

using SubroutineMap = std::unordered_map<Nes::Address, NesSubroutine*>;

// ���� NES ������������Ϣ�����ݿ���
class NesAnalyzer
{
public:
	NesAnalyzer(NesDataBase& db);
	~NesAnalyzer();

	void Analyze();
protected:
	// ���һ���ӳ����ӳ����
	void AddSubroutine(NesSubroutine* subroutine);
	// �ж�ָ����ַ�Ƿ������
	bool IsSubroutineAnalyzed(Nes::Address addr);

	// �����ӳ���
	NesSubroutine* AnalyzeSubroutine(NesSubroutineParser& parser, Nes::Address addr);
	// ����ӳ���ĵ��ù�ϵ
	void DumpCallRelation(NesSubroutine* subroutine);
	// ��������ӳ���ĵ��ù�ϵ
	void DumpAllCallRelation();
	// �����ӳ����Ƿ�ʹ����AXY��Ϊ�������Ƿ�ʹ��AXY����ֵ
	void AnalyzeSubroutineRegisterAXY();
	void AnalyzeTACSubroutine(TACSubroutine* subroutine);
protected:
	NesDataBase& db;
	Allocator allocator;
	SubroutineMap subMap;
};

