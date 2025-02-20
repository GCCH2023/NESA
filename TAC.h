#pragma once
#include "NesObject.h"


// 操作码
enum class TACOperator
{
	NOP,  // 占位
	ADD,  // 加法
	SUB,  // 减法
	ASSIGN, // 赋值 z = x
	BOR,  // 位或 |, z = x | y
	BAND,  // 位与 &, z = x & y
	BNOT,  // 取反 z = ~x
	XOR, // 异或 z = x ^ y
	IFGREAT,  // if x > y goto z
	IFGEQ,  // if x >= y goto z
	IFLESS,  // if x < y goto z
	IFLEQ,  // if x <= y goto z
	IFNEQ,  // if x != y goto z
	IFEQ,  // if x == y goto z
	INDEX,  // 索引 z = x[y]
	REF,  // 解引用 x[y] = z
	ARG,  // 传递一个函数参数，相当于 x86 中的 push x
	CALL, // z = x(y)，x 是函数地址，y是参数数量
	GOTO, // goto z
	RETURN,  // return
	SHR,  // 逻辑右移，z = x >> y，丢弃最低位
	SHL,  // 逻辑左移，z = x << y，丢弃最高位
	ROR,  // 循环右移，z = x >> y，最低位变为最高位
	ROL,  // 循环左移，z = x << y，最高位变为最低位
	PUSH, // x 入栈
	POP,  // 出栈到 z

	BREAK,  // brk
	BIT,  // bit，还不知道该怎么翻译（不知道这个指令的用法）
};

const TCHAR* ToString(TACOperator op);

// 操作数
// 与 NES 地址和寄存器相关
class TACOperand
{
public:
	enum
	{
		INTEGER = 0,  // 整数常量, 值是8位整数值
		ADDRESS = 0x01000000,  // 内存地址，值是16位地址
		MEMORY = 0x10000000,  // 内存单元，值是16位地址
		REGISTER = 0x11000000,  // 寄存器，值是寄存器编号

		TEMP = 0x00100000,  // 临时变量，值是它的编号

		VALUE_MASK = 0xFFFF,
		KIND_MASK = 0xFF000000,
	};
	TACOperand();
	TACOperand(uint32_t value);
	bool operator==(const TACOperand& other);
	// 是否临时变量
	inline bool IsTemp() const { return (data & TEMP) != 0; }
	// 是否寄存器
	inline bool IsRegister() const { return GetKind() == REGISTER; }
	// 是否内存地址
	inline bool IsAddress() const { return GetKind() == ADDRESS; }
	// 是否整数
	inline bool IsInterger() const { return GetKind() == INTEGER; }
	// 是否内存单元
	inline bool IsMemory() const { return GetKind() == MEMORY; }
	// 获取值
	inline uint32_t GetValue() const { return data & VALUE_MASK; }
	// 设置值
	inline void SetValue(int value) { data |= value & VALUE_MASK; }
	// 获取类别
	inline uint32_t GetKind() const { return data & KIND_MASK; }
	// 设置类别
	inline void SetKind(uint32_t kind) { data = (data & ~KIND_MASK) | kind; }
	// 是否是零
	inline bool IsZero() const { return data == 0; }
private:
	uint32_t data;
};

OStream& operator<<(OStream& os, const TACOperand& obj);

// NES 寄存器 操作数
extern TACOperand RegisterP;
extern TACOperand RegisterA;
extern TACOperand RegisterX;
extern TACOperand RegisterY;
extern TACOperand RegisterSP;

// 三地址码
// 包含一个操作码和三个操作数
// x 和 y 作为运算操作数，z 作为结果
class TAC
{
public:
	TAC(TACOperator op);
	TAC(TACOperator op, TACOperand z);
	TAC(TACOperator op, TACOperand z, TACOperand x);
	TAC(TACOperator op, TACOperand z, TACOperand x, TACOperand y);
public:
	uint32_t address;
	TACOperator op;
	TACOperand x;
	TACOperand y;
	TACOperand z;
};

// 计算常量操作数的值
// 如果不是常量抛出异常
// 如果是单目操作码的话，y的值应该是整数0
TACOperand Evaluate(TACOperator op, TACOperand x, TACOperand y);
OStream& operator<<(OStream& os, const TAC* obj);
// 输出带地址的三地址码
OStream& DumpAddressTAC(OStream& os, const TAC* tac);


using TACList = std::vector<TAC*>;

// 三地址码的基本块
class TACBasicBlock : public NesRegion
{
public:
	TACBasicBlock();
	TACBasicBlock(Nes::Address startAddress, Nes::Address endAddress);

	// 添加一条三地址码
	inline void AddTAC(TAC* code) { codes.push_back(code); }
	inline TACList& GetCodes() { return codes; }
	// 获取三地址码的数量
	inline size_t GetCodesCount() const { return codes.size(); }
private:
	TACList codes;  // 三地址码列表
public:
	std::vector<TACBasicBlock*> prevs;
	std::vector<TACBasicBlock*> nexts;
	uint32_t flag;  // 一些标志
};

using TACBasicBlockList = std::vector<TACBasicBlock*>;

// 三地址码子程序
class TACSubroutine : public NesRegion
{
public:
	TACSubroutine();
	TACSubroutine(Nes::Address startAddress, Nes::Address endAddress);

	// 输出内容
	void Dump();

	// 获取所有三地址码
	TACList GetCodes();

	// 添加一个基本块
	inline void AddBasicBlock(TACBasicBlock* block) { blocks.push_back(block); }
	inline TACBasicBlockList& GetBasicBlocks() { return blocks; }

	// 添加一个新的临时变量，返回其编号
	// bytes 是它占用的字节数 (目前没用到）
	inline int NewTemp(int bytes) { return tempCount++; }

	// 获取AXY参数标志
	uint32_t GetParamFlag() const { return flag & 7; }
	// 获取AXY返回值标志
	uint32_t GetReturnFlag() const { return (flag >> 3) & 7; }

	uint32_t flag;  // 低3位表示AXY作为参数，3 - 5位表示AXY作为返回值
private:
	TACBasicBlockList blocks;  // 基本块列表
	int tempCount;  // 临时变量计数器
};

// 输出子程序对AXY的引用情况
void DumpTACSubroutineAXY(TACSubroutine* sub);

