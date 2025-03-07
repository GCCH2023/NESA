#pragma once
#include "NesObject.h"

class NesBasicBlock;

using BasicBlockList = std::vector<NesBasicBlock*>;

// 子程序的调用关系
struct CallRelation
{
	Nes::Address call;  // 调用方子程序地址
	Nes::Address called;  // 被调用子程序地址

	CallRelation();
	CallRelation(Nes::Address call, Nes::Address called);
	inline Nes::Address GetStartAddress() const { return call; }
	inline void SetStartAddress(Nes::Address addr) { call = addr; }
};

using CallList = std::vector<CallRelation*>;

enum SubroutineFlag
{
	SUBF_PARAM_A = 1,  // 使用 A 寄存器作为参数
	SUBF_PARAM_X = 2,  // 使用 X 寄存器作为参数
	SUBF_PARAM_Y = 4,  // 使用 Y 寄存器作为参数
	SUBF_PARAM = 7,  // 使用了 AXY 寄存器作为参数
	SUBF_RETURN_A = 8,  // 使用 A 寄存器作为返回值
	SUBF_RETURN_X = 0x10,  // 使用 X 寄存器作为返回值
	SUBF_RETURN_Y = 0x20,  // 使用 Y 寄存器作为返回值
	SUBF_RETURN = 0x38,  // 使用了 AXY 寄存器作为返回值
	SUBF_LOCAL_A = 0x40,  // 使用 A 寄存器作为局部变量
	SUBF_LOCAL_X = 0x80,  // 使用 X 寄存器作为局部变量
	SUBF_LOCAL_Y = 0x100,  // 使用 Y 寄存器作为局部变量
	SUBF_LOCAL = 0x1C0,  // 使用了 AXY 寄存器作为局部变量
};

class NesSubroutine : public NesRegion
{
public:
	NesSubroutine();
	NesSubroutine(Nes::Address startAddr, Nes::Address endAddr);

	// 按地址从小到大地添加基本块，重复地址不添加
	void AddBasicBlock(NesBasicBlock* block);
	const BasicBlockList& GetBasicBlocks() const { return blocks; }
	// 根据地址查找基本块
	NesBasicBlock* FindBasicBlock(Nes::Address addr);

	// 获取调用的子程序列表
	inline std::vector<Nes::Address>& GetCalls() { return calls; }
	// 添加一个调用子程序地址
	void AddCall(Nes::Address addr);

	// 输出子程序的信息
	void Dump();

	// 获取AXY参数标志
	uint32_t GetParamFlag() const { return flag & 7; }
	// 获取AXY返回值标志
	uint32_t GetReturnFlag() const { return (flag >> 3) & 7; }

	uint32_t flag;  // 低3
protected:
	BasicBlockList blocks;  // 子程序包括的基本块列表，按地址从小到大排列
	std::vector<Nes::Address> calls;  // 调用的子程序地址，如果为空，则此子程序没有调用其他子程序，从小到大排列
};

