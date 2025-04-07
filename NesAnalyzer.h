#pragma once
class NesDataBase;
class NesSubroutine;
class TACFunction;


// 解析 NES 卡带，保存信息到数据库中
class NesAnalyzer
{
public:
	NesAnalyzer(NesDataBase& db);
	~NesAnalyzer();

	void Analyze();
protected:
	// 根据地址查找子程序
	NesSubroutine* FindSubroutine(Nes::Address address);

	// 分析所有的子程序
	void AnalyzeSubroutine();
	// 输出子程序的调用关系
	void DumpCallRelation(NesSubroutine* subroutine);
	// 输出所有子程序的调用关系
	void DumpAllCallRelation();
	// 分析子程序是否使用了AXY作为参数，是否使用AXY返回值
	void AnalyzeSubroutineRegisterAXY();
	
	inline const std::vector<NesSubroutine*>& GetSubroutines() const { return subroutines; }
protected:
	NesDataBase& db;
	Allocator allocator;
	std::vector<NesSubroutine*> subroutines;  // 当前分析的所有函数
};

