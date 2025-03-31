#pragma once
#include "TACFunction.h"
#include "NesDataBase.h"

struct Type;

// 将 NES 代码转换为 NES 三地址码
// TAC 子程序，基本块，指令都在分配器上分配
// 翻译一条指令时，该指令影响的标志位也会翻译
class TACTranslater1
{
public:
	TACTranslater1(NesDataBase& db, Allocator& allocator);
	~TACTranslater1();
	// 将 NES 子程序转换为 三地址码表示的子程序
	TACFunction* Translate(NesSubroutine* subroutine);
	// 重置数据，可以复用对象
	void Reset();

protected:
	TACBasicBlock* TranslateBasickBlock(NesBasicBlock* block);
	// 获取指令的操作数
	// 对于复杂的寻址模式，可能会设置多个操作数
	TACOperand GetOperand(const Instruction& instruction);
	// 创建一个新的临时变量
	TACOperand NewTemp(Type* type);
	// 解析操作数，返回是否多个操作数
	bool TranslateOperand(TAC& tac, const Instruction& instruction);
	// 添加一条三地址码
	void AddTAC(TAC* tac, Nes::Address address);
	// 记录三地址码的开始索引
	void SaveTACStart();
	// 根据基本块中NES指令的索引，获取对应的三地址码
	TAC* GetBlockTAC(int index);
	// 翻译函数调用指令
	TAC* TranslateCall(Nes::Address callAddr, Nes::Address addr);
	// 翻译函数返回指令
	// address: 返回指令的地址
	TAC* TranslateReturn(Nes::Address address);
	// 翻译条件跳转指令
	TAC* TranslateJump(TACOperator op, const Instruction& instruction, TACOperand flag);
	// 获取AXY结构体变量的索引
	int GetAxy();
	// 翻译NES指令影响的标志位NC，其他标志位需要特殊处理
	// instruction : 当前翻译的NES指令
	// tac : 该指令对应的三地址码
	void TranslateFlag(const Instruction& intruction, TAC* tac);
	// 获取共享的临时变量
	TACOperand GetSharedTemp();
	// 生成尾函数调用
	// target: 要调用的函数操作数
	TAC* GenerateTailCall(TACOperand target, Nes::Address address);
protected:
	NesDataBase& db;
	Allocator& allocator;
	NesSubroutine* nesSub;
	TACFunction* tacSub;
	TACBasicBlock* tacBlock;
	// 在将一个基本块的NES指令翻译为三地址码的过程中，
	// 记录下每条指令对应的三地址码的开始索引
	std::vector<int> tacStarts;
	int axy;  // 返回值临时变量编号
	TACOperand temp;  // 共享的临时变量
};

