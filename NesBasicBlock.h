#pragma once
#include "NesObject.h"

enum BasicBlockFlag
{
	BBF_END_NORMAL = 0,  // 以普通指令结束，顺序转到后面基本块
	BBF_END_RETURN = 1,  // 以返回指令结束
	BBF_END_COND = 2,  // 以条件跳转指令结束
	BBF_END_UNCOND = 3,  // 以无条件跳转指令结束
	BBF_END_MASK = 3,  // 检索结束标志的掩码

	// 以跳转指令结束时的标志
	BBF_JUMP_BEFOER = 4,  // 条件跳转到低地址，没有此标志则是跳转到高地址

	BBF_JUMP_SELF = 8,  // 跳转到自己

	BBF_ENTRY = 16,  // 函数入口基本块
};

// 基本块的一些特性
// 1. 如果自己是自己的前驱，那么它自己构成一个循环
// 2. 一个基本块最多有两个后继块：
//    (1) 如果没有后继，则说明是函数结束
//    (2) 如果有1个后继，则是以无条件跳转指令结束
//    (3) 如果有两个后继，则是以条件跳转指令结束
// 3. 一个基本块如果是往前跳转（低地址），则很可能是循环语句；
//  往后跳转则必定是分支语句
class NesBasicBlock : public NesRegion
{
public:
	NesBasicBlock();
	NesBasicBlock(Nes::Address startAddr, Nes::Address endAddr);
	NesBasicBlock(const NesBasicBlock& other);

	void AddPrev(NesBasicBlock* block);
	// 获取后继的数量
	size_t GetNextCount() const;
	// 获取前驱的数量
	size_t GetPrevCount() const { return prevs.size(); }
	// 输出信息
	void Dump();
public:
	std::list<NesBasicBlock*> prevs; // 前驱块
	NesBasicBlock* nexts[2];  // 后继块
	uint32_t flag;  // 一些标志
};

