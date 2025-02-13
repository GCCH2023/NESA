#pragma once
#include "Cartridge.h"
#include "Instruction.h"
#include "NesSubroutine.h"
#include "NesBasicBlock.h"

using InstructionList = std::vector<Instruction>;
using SubroutineList = std::vector<NesSubroutine*>;

// NES 数据库
class NesDataBase
{
public:
	NesDataBase(const TCHAR* rom);
	~NesDataBase();

	// 获取 RESET 中断处理程序的地址
	inline Nes::Address GetInterruptResetAddress() const { return reset; }
	// 获取 NMI 中断处理程序的地址
	inline Nes::Address GetInterruptNmiAddress() const { return nmi; }
	// 获取 IRQ 中断处理程序的地址
	inline Nes::Address GetInterruptIrqAddress() const { return irq; }

	// 获取卡带对象
	inline Cartridge& GetCartridge() { return cartridge; }

	// 根据地址获取指令
	const Instruction* GetInstruction(Nes::Address address);
	// 获取指定地址范围内的指令
	void GetInstructions(InstructionList& list, Nes::Address begin, Nes::Address end);

	// 有序地添加基本块
	void AddBasicBlock(NesBasicBlock* block);
	// 根据地址查找基本块，该基本块包含指定地址
	NesBasicBlock* FindBasicBlock(Nes::Address address);
	// 获取指定地址区间内的基本块列表
	BasicBlockList GetBasicBlocks(Nes::Address start, Nes::Address end);

	// 有序地添加子程序
	void AddSubroutine(NesSubroutine* subroutine);
	// 根据地址查找子程序
	NesSubroutine* FindSubroutine(Nes::Address address);
	// 获取子程序列表
	SubroutineList& GetSubroutines() { return subroutines; }

	// 有序地添加子程序调用关系
	void AddCallRelation(CallRelation* call);

	Allocator allocator;

private:
	// 卡带
	Cartridge cartridge;

	// 三个中断向量的处理函数地址
	Nes::Address nmi;
	Nes::Address reset;
	Nes::Address irq;

	BasicBlockList basicBlocks;  // 基本块表，按地址从小到大排列
	CallList calls;  // 函数调用关系

	SubroutineList subroutines;  // 子程序列表
	std::vector<Instruction*> instructions;  // 指令表
};

