#pragma once
class NesBasicBlock;
class NesDataBase;

// 解析指定区域的NES指令为基本块列表
// 基本块按地址从小到大排列
class NesBasicBlockParser
{
public:
	NesBasicBlockParser(NesDataBase& db, Allocator& allocator);
	~NesBasicBlockParser();
	std::vector<NesBasicBlock*> Parse(Nes::Address start, Nes::Address end);
	void Reset();
protected:
	void AddBlock(uint32_t start, uint32_t end);
protected:
	NesDataBase& db;
	Allocator& allocator;
	std::map<uint32_t, NesBasicBlock*> addrBlockMap;  // 键基本块开始地址
};

