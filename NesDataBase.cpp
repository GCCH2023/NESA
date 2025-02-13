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

void NesDataBase::AddBasicBlock(NesBasicBlock* block)
{
	AddNesObject(basicBlocks, block);
}

NesBasicBlock* NesDataBase::FindBasicBlock(Nes::Address address)
{
	return FindNesObject(basicBlocks, address);
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

void NesDataBase::AddSubroutine(NesSubroutine* subroutine)
{
	AddNesObject(subroutines, subroutine);
}

NesSubroutine* NesDataBase::FindSubroutine(Nes::Address address)
{
	return FindNesObject(subroutines, address);
}

void NesDataBase::AddCallRelation(CallRelation* call)
{
	AddNesObject(calls, call);
}