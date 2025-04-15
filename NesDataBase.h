#pragma once
#include "Cartridge.h"
#include "Instruction.h"
#include "NesSubroutine.h"
#include "NesBasicBlock.h"

class Allocator;

using InstructionList = std::vector<Instruction>;
using SubroutineList = std::vector<NesSubroutine*>;

// 交叉引用类型枚举
enum class XRefType
{
	CodeCall,      // 函数调用
	CodeJump,      // 跳转
	DataRead,      // 数据读取
	DataWrite,     // 数据写入
};

struct XRef
{
	Nes::Address address;
	XRefType type;
};

using XRefList = std::vector<XRef>;

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
	// 获取子程序包含的所有指令
	void GetInstructions(InstructionList& list, NesSubroutine* subroutine);

	// 有序地添加基本块
	void AddBasicBlock(NesBasicBlock* block);
	// 获取开始地址为指定地址的基本块
	NesBasicBlock* GetBasicBlock(Nes::Address startAddress);
	// 根据地址查找基本块，该基本块包含指定地址
	NesBasicBlock* FindBasicBlock(Nes::Address address);
	// 获取指定地址区间内的基本块列表
	BasicBlockList GetBasicBlocks(Nes::Address start, Nes::Address end);
	// 根据地址获取获取包含这个地址的基本块或者这个地址后面的第一个基本块
	NesBasicBlock* GetBasicBlockOrNext(Nes::Address address);

	// 有序地添加子程序
	void AddSubroutine(NesSubroutine* subroutine);
	// 根据子程序开始地址查找子程序
	NesSubroutine* FindSubroutine(Nes::Address address);
	// 获取子程序列表
	SubroutineList& GetSubroutines() { return subroutines; }
	// 根据地址获取获取包含这个地址的子程序
	NesSubroutine* GetSubroutine(Nes::Address address);
	// 根据地址获取获取包含这个地址的子程序或者这个地址后面的第一个子程序
	NesSubroutine* GetSubroutineOrNext(Nes::Address address);

	// 添加交叉引用
	void AddXref(XRefType type, Nes::Address from, Nes::Address to);
	// 获取从指定地址发出的所有引用
	const XRefList GetXrefsFrom(Nes::Address from) const;
	// 获取指向指定地址的所有引用
	const  XRefList GetXrefsTo(Nes::Address to) const;
	// 获取特定类型的交叉引用
	XRefList GetXrefsTo(Nes::Address to, XRefType type) const;
	// 获取交叉引用总数
	size_t GetXrefsCount() const;


	// 获取分配器
	inline Allocator& GetAllocator() { return allocator; }


private:
	Allocator allocator;
	// 卡带
	Cartridge cartridge;

	// 三个中断向量的处理函数地址
	Nes::Address nmi;
	Nes::Address reset;
	Nes::Address irq;

	BasicBlockList basicBlocks;  // 基本块表，按地址从小到大排列
	SubroutineList subroutines;  // 子程序列表

	std::map<Nes::Address, XRefList> xrefsFrom;  // 源地址到目标地址的映射
	std::map<Nes::Address, XRefList> xrefsTo;    // 目标地址到来源地址的映射

	std::vector<Instruction*> instructions;  // 指令表
};

