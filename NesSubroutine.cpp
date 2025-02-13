#include "stdafx.h"
#include "NesSubroutine.h"
#include "NesBasicBlock.h"

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
	// 使用 std::lower_bound 查找插入位置
	auto it = std::lower_bound(blocks.begin(), blocks.end(), block, [](NesBasicBlock* a, NesBasicBlock* b) {
		return a->GetStartAddress() < b->GetStartAddress();
	});

	// 检查该位置是否已经存在相同的值
	if (it == blocks.end() || (*it)->GetStartAddress() != block->GetStartAddress())
		// 如果不存在相同的值，则插入新值
		blocks.insert(it, block);
}

NesBasicBlock* NesSubroutine::FindBasicBlock(Nes::Address addr)
{
	auto it = std::lower_bound(blocks.begin(), blocks.end(), addr, [](NesBasicBlock* a, Nes::Address addr) {
		return a->GetStartAddress() < addr;
	});
	if (it != blocks.end() && (*it)->GetStartAddress() == addr)
		return *it;
	return nullptr;
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
