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
	using BlockList = std::vector<Nes::Address>;

	NesBasicBlock();
	NesBasicBlock(Nes::Address startAddr, Nes::Address endAddr);
	NesBasicBlock(const NesBasicBlock& other);

	inline void AddPred(Nes::Address addr) { preds.push_back(addr); }
	void AddSucc(Nes::Address addr) { succs.push_back(addr); }
	inline const BlockList& GetPreds() { return preds; }
	inline const BlockList& GetSuccs() { return succs; }
	// 获取后继的数量
	inline size_t GetSuccsCount() const { return succs.size(); }
	// 获取前驱的数量
	inline size_t GetPredsCount() const { return preds.size(); }
	inline void ClearPreds() { preds.clear(); }
	inline void ClearSuccs() { succs.clear(); }
	// 从指定地址分割，后半部分设置为指定基本块
	// 指定地址必须位于此基本块中间(大于开始地址且小于结束地址)
	void Split(Nes::Address addr, NesBasicBlock* succ);
	// 判断指定地址是否是前驱
	bool IsPred(Nes::Address addr) const;
	// 判断指定地址是否是后继
	bool IsSucc(Nes::Address addr) const;
	// 设置结束标志
	void SetEndFlag(BasicBlockFlag endFlag)
	{
		flag = (flag & ~BBF_END_MASK) | (endFlag & BBF_END_MASK);
	}
	// 输出信息
	void Dump();

	uint32_t flag;  // 一些标志
private:
	BlockList preds; // 前驱列表
	BlockList succs;  // 后继列表
};

