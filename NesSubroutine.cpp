#include "stdafx.h"
#include "NesSubroutine.h"
#include "NesBasicBlock.h"
#include "NesUtil.h"

NesSubroutine::NesSubroutine()
{

}

NesSubroutine::NesSubroutine(Nes::Address startAddr, Nes::Address endAddr):
NesRegion(startAddr, endAddr)
{
}

void NesSubroutine::AddBasicBlock(NesBasicBlock* block)
{
	if (!block)
		return;
	blocks.push_back(block);
}

NesBasicBlock* NesSubroutine::GetBasicBlock(Nes::Address addr)
{
	for (auto block : blocks)
	{
		if (block->GetStartAddress() == addr)
			return block;
	}
	return nullptr;
}

// 获取入口基本块

NesBasicBlock* NesSubroutine::GetEntryBasicBlock()
{
	if (blocks.empty())
		return nullptr;
	return GetBasicBlock(GetStartAddress());
}

void NesSubroutine::Clear()
{
	this->blocks.clear();
	this->calls.clear();
}

void NesSubroutine::AddCall(Nes::Address addr)
{
	auto it = std::lower_bound(calls.begin(), calls.end(), addr);
	// 检查该位置是否已经存在相同的值
	if (it == calls.end() || *it != addr)
		// 如果不存在相同的值，则插入新值
		calls.insert(it, addr);
}

void NesSubroutine::Dump()
{

}

bool NesSubroutine::Contains(Nes::Address addr)
{
	for (auto block : GetBasicBlocks())
	{
		if (block->Contains(addr))
			return true;
	}
	return false;
}

CallRelation::CallRelation():
call(0),
called(0)
{

}

CallRelation::CallRelation(Nes::Address call_, Nes::Address called_):
call(call_),
called(called_)
{

}
