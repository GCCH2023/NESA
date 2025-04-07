#pragma once

class NesDataBase;

struct SubroutineRange
{
	Nes::Address start;
	Nes::Address end;
};

// 尽可能多的识别NES子程序的范围，考虑内联函数的情况
// 不会将子程序信息写入数据库
class NesSubroutineRangeParser
{
public:
	NesSubroutineRangeParser(NesDataBase& db);
	~NesSubroutineRangeParser();
	// 从给定地址开始，递归地解析子程序的范围，返回解析出来的所有子程序范围
	// 子程序按地址从小到大排列
	std::vector<SubroutineRange>& Parse();
protected:
	// 解析指定地址的子程序的范围
	void ParseSubroutine(Nes::Address address);
	//// 判断指定地址是否在子程序前面
	//inline bool IsBackAddress(Nes::Address address) const { return address < subroutineAddress; }
	// 添加一个函数地址到队列中
	inline void AddSubroutineAddress(Nes::Address address) { subAddrs.push_back(address); }
	// 添加一个函数
	inline void AddSubroutine(Nes::Address start, Nes::Address end);
	// 根据地址获取获取包含这个地址的子程序或者这个地址后面的第一个子程序
	SubroutineRange* GetSubroutineOrNext(Nes::Address address);
	// 根据给定范围分析子程序范围
	SubroutineRange ParseSubroutineRange(uint32_t start, uint32_t end);
	void Reset();
private:
	NesDataBase& db;

	std::vector<SubroutineRange> subroutines;  // 记录每个函数的开始地址和结束地址，从小到大排序
	std::vector<Nes::Address> subAddrs;  // 函数开始地址队列
};

