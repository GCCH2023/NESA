#include "stdafx.h"
#include "NesDataBase.h"
#include "NesUtil.h"

NesDataBase::NesDataBase(const TCHAR* rom)
{
	cartridge.LoadRom(rom);

	nmi = cartridge.ReadWord(Nes::InterruptVector::NMI);
	reset = cartridge.ReadWord(Nes::InterruptVector::RESET);
	irq = cartridge.ReadWord(Nes::InterruptVector::IRQ);
}


NesDataBase::~NesDataBase()
{
}

const Instruction* NesDataBase::GetInstruction(Nes::Address address)
{
	uint32_t index = address - 0x8000;
	if (instructions[index])
		return instructions[index];

	auto p = cartridge.GetData(address);
	Instruction* instruction = allocator.New<Instruction>();
	instruction->Set(address, p);
	instructions[index] = instruction;
	return instruction;
}

void NesDataBase::GetInstructions(InstructionList& list, Nes::Address begin, Nes::Address end)
{
	Instruction instruction;
	for (auto addr = begin; addr < end;)
	{
		auto p = cartridge.GetData(addr);
		instruction.Set(addr, p);
		int bytes = instruction.GetLength();
		p += bytes;
		addr += bytes;

		list.push_back(instruction);
	}
}

void NesDataBase::GetInstructions(InstructionList& list, NesSubroutine* subroutine)
{
	for (auto block : subroutine->GetBasicBlocks())
	{
		GetInstructions(list, block->GetStartAddress(), block->GetEndAddress());
	}
}

void NesDataBase::AddBasicBlock(NesBasicBlock* block)
{
	AddNesObject(basicBlocks, block);
}

NesBasicBlock* NesDataBase::GetBasicBlock(Nes::Address address)
{
	return FindNesObject(basicBlocks, address);
}

NesBasicBlock* NesDataBase::FindBasicBlock(Nes::Address address)
{
	// 使用二分查找快速定位
	auto it = std::upper_bound(basicBlocks.begin(), basicBlocks.end(), address,
		[](Nes::Address addr, const NesBasicBlock* block) {
			return addr < block->GetStartAddress();
		});

	if (it != basicBlocks.begin()) {
		--it; // 回退到可能包含该地址的块
		if ((*it)->Contains(address))
			return *it;
	}

	return nullptr; // 没有找到包含该地址的块
}

BasicBlockList NesDataBase::GetBasicBlocks(Nes::Address start, Nes::Address end)
{
	BasicBlockList list;
	for (auto block : basicBlocks)
	{
		if (block->GetStartAddress() >= start && block->GetEndAddress() <= end)
		{
			list.push_back(block);
		}
	}
	return list;
}

NesBasicBlock* NesDataBase::GetBasicBlockOrNext(Nes::Address address)
{
	// 查找第一个结束地址大于指定地址的子程序
	auto it = std::lower_bound(basicBlocks.begin(), basicBlocks.end(), address,
		[](const NesBasicBlock* a, Nes::Address address) {
			return a->GetEndAddress() <= address;
		});
	// 如果该子程序包含指定地址，则返回它，否则返回空
	if (it == basicBlocks.end())
		return nullptr;
	return *it;
}

void NesDataBase::AddSubroutine(NesSubroutine* subroutine)
{
	AddNesObject(subroutines, subroutine);
}

NesSubroutine* NesDataBase::FindSubroutine(Nes::Address address)
{
	return FindNesObject(subroutines, address);
}

NesSubroutine* NesDataBase::GetSubroutine(Nes::Address address)
{
	// 查找第一个结束地址大于指定地址的子程序
	auto it = std::lower_bound(subroutines.begin(), subroutines.end(), address,
		[](NesSubroutine* a, Nes::Address address){
		return a->GetEndAddress() <= address;
	});
	// 如果该子程序包含指定地址，则返回它，否则返回空
	if (it != subroutines.end() && (*it)->GetStartAddress() <= address)
		return *it;
	return nullptr;
}

NesSubroutine* NesDataBase::GetSubroutineOrNext(Nes::Address address)
{
	// 查找第一个结束地址大于指定地址的子程序
	auto it = std::lower_bound(subroutines.begin(), subroutines.end(), address,
		[](NesSubroutine* a, Nes::Address address){
		return a->GetEndAddress() <= address;
	});
	// 如果该子程序包含指定地址，则返回它，否则返回空
	if (it == subroutines.end())
		return nullptr;
	return *it;
}

void NesDataBase::AddCallRelation(CallRelation* call)
{
	AddNesObject(calls, call);
}

