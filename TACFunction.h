#pragma once
#include "TAC.h"

using TACList = std::vector<TAC*>;
struct Type;

// 三地址码的基本块
class TACBasicBlock : public NesRegion
{
public:
	TACBasicBlock();
	TACBasicBlock(Nes::Address startAddress, Nes::Address endAddress);

	// 添加一条三地址码
	inline void AddTAC(TAC* code) { codes.push_back(code); }
	inline TACList& GetCodes() { return codes; }
	// 获取三地址码的数量
	inline size_t GetCodesCount() const { return codes.size(); }
private:
	TACList codes;  // 三地址码列表
public:
	std::vector<TACBasicBlock*> prevs;
	std::vector<TACBasicBlock*> nexts;
	uint32_t flag;  // 一些标志
};

using TACBasicBlockList = std::vector<TACBasicBlock*>;


// 三地址码子程序
class TACFunction : public NesRegion
{
public:
	TACFunction();
	TACFunction(Nes::Address startAddress, Nes::Address endAddress);

	// 输出内容
	void Dump();

	// 获取所有三地址码
	TACList GetCodes();

	// 添加一个基本块
	inline void AddBasicBlock(TACBasicBlock* block) { blocks.push_back(block); }
	inline TACBasicBlockList& GetBasicBlocks() { return blocks; }

	// 添加一个新的临时变量，返回其编号
	// bytes 是它占用的字节数 (目前没用到）
	int NewTemp(Type* type);

	// 获取AXY参数标志
	uint32_t GetParamFlag() const { return flag & 7; }
	// 获取AXY返回值标志
	uint32_t GetReturnFlag() const { return (flag >> 3) & 7; }

	uint32_t flag;  // 低3位表示AXY作为参数，3 - 5位表示AXY作为返回值
private:
	TACBasicBlockList blocks;  // 基本块列表
	std::vector<Type*> tempTypes;  // 临时变量的类型
};

// 输出子程序对AXY的引用情况
void DumpTACSubroutineAXY(TACFunction* sub);