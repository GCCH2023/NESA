#pragma once
#include "TAC.h"
#include "NesDataBase.h"

// 将 NES 代码转换为 NES 三地址码
class TACTranslater
{
public:
	TACTranslater(NesDataBase& db, Allocator& allocator);
	~TACTranslater();
	// 将 NES 子程序转换为 三地址码表示的子程序
	TACSubroutine* Translate(NesSubroutine* subroutine);
protected:
	TACBasicBlock* TranslateBasickBlock(NesBasicBlock* block);
	// 获取指令的操作数
	TACOperand GetOperand(const Instruction& instruction);
	// 翻译 CMP, CPX, CPY 指令
	// reg 指示是哪个寄存器
	TAC* TranslateCmp(const Instruction& instruction, const Instruction& next, TACOperand reg);
	// 添加一条三地址码
	void AddTAC(TAC* tac);
private:
	NesDataBase& db;
	Allocator& allocator;
	NesSubroutine* nesSub;
	TACSubroutine* tacSub;
	TACBasicBlock* tacBlock;
};

