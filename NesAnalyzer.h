#pragma once
class NesDataBase;
class NesSubroutine;
class NesSubroutineParser;
class TACSubroutine;

using SubroutineMap = std::unordered_map<Nes::Address, NesSubroutine*>;

// 解析 NES 卡带，保存信息到数据库中
class NesAnalyzer
{
public:
	NesAnalyzer(NesDataBase& db);
	~NesAnalyzer();

	void Analyze();
protected:
	// 添加一个子程序到子程序表
	void AddSubroutine(NesSubroutine* subroutine);
	// 判断指定地址是否分析过
	bool IsSubroutineAnalyzed(Nes::Address addr);

	// 分析子程序
	NesSubroutine* AnalyzeSubroutine(NesSubroutineParser& parser, Nes::Address addr);
	// 输出子程序的调用关系
	void DumpCallRelation(NesSubroutine* subroutine);
	// 输出所有子程序的调用关系
	void DumpAllCallRelation();
	// 分析子程序是否使用了AXY作为参数，是否使用AXY返回值
	void AnalyzeSubroutineRegisterAXY();
	void AnalyzeTACSubroutine(TACSubroutine* subroutine);
protected:
	NesDataBase& db;
	Allocator allocator;
	SubroutineMap subMap;
};

