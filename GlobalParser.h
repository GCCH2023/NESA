#pragma once
class NesSubroutine;
class NesDataBase;

// ����ȫ�ֱ���
class GlobalParser
{
public:
	GlobalParser(NesDataBase& db);
	~GlobalParser();
	void Parse(NesSubroutine* subroutine);
protected:
	void Reset();

protected:
	NesDataBase& db;
};

