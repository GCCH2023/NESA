#pragma once
#include "TAC.h"
#include "NesDataBase.h"

// 将 NES 代码转换为 NES 三地址码
// TAC 子程序，基本块，指令都在分配器上分配
class TACTranslater
{
public:
	TACTranslater(NesDataBase& db, Allocator& allocator);
	~TACTranslater();
	// 将 NES 子程序转换为 三地址码表示的子程序
	TACSubroutine* Translate(NesSubroutine* subroutine);
	// 重置数据，可以复用对象
	void Reset();

protected:
	TACBasicBlock* TranslateBasickBlock(NesBasicBlock* block);
	// 获取指令的操作数
	TACOperand GetOperand(const Instruction& instruction);
	// 翻译 CMP, CPX, CPY 指令
	// reg 指示是哪个寄存器
	TAC* TranslateCmp(const Instruction& instruction, const Instruction& next, TACOperand reg);
	// 添加一条三地址码
	void AddTAC(TAC* tac, Nes::Address address);
	// 记录三地址码的开始索引
	void SaveTACStart();
	// 根据基本块中NES指令的索引，获取对应的三地址码
	TAC* GetBlockTAC(int index);
	// 翻译条件跳转指令
	TAC* TranslateConditionalJump(std::vector<Instruction>& instructions, size_t index, uint32_t need, TACOperator op);
private:
	NesDataBase& db;
	Allocator& allocator;
	NesSubroutine* nesSub;
	TACSubroutine* tacSub;
	TACBasicBlock* tacBlock;
	// 在将一个基本块的NES指令翻译为三地址码的过程中，
	// 记录下每条指令对应的三地址码的开始索引
	std::vector<int> tacStarts;
};

