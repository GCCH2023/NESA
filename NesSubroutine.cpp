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
	// ʹ�� std::lower_bound ���Ҳ���λ��
	auto it = std::lower_bound(blocks.begin(), blocks.end(), block, [](NesBasicBlock* a, NesBasicBlock* b) {
		return a->GetStartAddress() < b->GetStartAddress();
	});

	// ����λ���Ƿ��Ѿ�������ͬ��ֵ
	if (it == blocks.end() || (*it)->GetStartAddress() != block->GetStartAddress())
		// �����������ͬ��ֵ���������ֵ
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
	// ����λ���Ƿ��Ѿ�������ͬ��ֵ
	if (it == calls.end() || *it != addr)
		// �����������ͬ��ֵ���������ֵ
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
