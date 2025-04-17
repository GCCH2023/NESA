#pragma once
class NesSubroutine;
class NesDataBase;
struct Instruction;

// 解析全局变量和交叉引用
class GlobalParser
{
public:
	GlobalParser(NesDataBase& db);
	~GlobalParser();
	void Parse(NesSubroutine* subroutine);
protected:
	void Reset();
	void ParseXRef(const Instruction& instruction);
protected:
	NesDataBase& db;
};

