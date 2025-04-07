#pragma once
class NesDataBase;
class NesSubroutine;
class NesSubroutineParser;

using SubroutineMap = std::unordered_map<Nes::Address, NesSubroutine*>;

// 递归地解析指定函数及其调用的函数
class NesSubroutinesParser
{
public:
	NesSubroutinesParser(NesDataBase& db);
	~NesSubroutinesParser();
	std::vector<NesSubroutine*>& Parse(Nes::Address address);

protected:
	NesSubroutine* ParseSubroutine(Nes::Address address, NesSubroutineParser& subParser);
	// 添加一个子程序到子程序表
	void AddSubroutine(NesSubroutine* subroutine);
protected:
	NesDataBase& db;
	SubroutineMap subMap;  // 当前分析的所有函数 地址-> 函数 表
	std::vector<NesSubroutine*> subroutines;  // 当前分析的所有函数
};

