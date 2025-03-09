#pragma once
class NesSubroutine;
class NesDataBase;

// 解析全局变量
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

