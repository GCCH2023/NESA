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
	ARRAY_GET,  // 获取数组元素 z = x[y]，x 是数组基地址，y 是元素的偏移量字节数
	ARRAY_SET,  // 设置数组元素 x[y] = z，x 是数组基地址，y 是元素的偏移量字节数
	ADDR,  // 取地址，z = &x
	DEREF,  // 解引用，若 z 是T类型，则x 是T指针类型，z = *x
	CAST,  // 类型转换, z = (T)x，T由符号表中的z类型给出
	ARG,  // 传递一个函数参数，相当于 x86 中的 push x
	CALL, // z = x(y)，x 是函数地址，y是参数数量
	GOTO, // goto z
	RETURN,  // return x
	SHR,  // 逻辑右移，z = x >> y，丢弃最低位
	SHL,  // 逻辑左移，z = x << y，丢弃最高位
	ROR,  // 循环右移，z = x >> y，最低位变为最高位
	ROL,  // 循环左移，z = x << y，最高位变为最低位
	PUSH, // x 入栈
	POP,  // 出栈到 z

	// 与 6502 相关的操作码
	BREAK,  // brk
	BIT,  // bit，还不知道该怎么翻译（不知道这个指令的用法）
	CLI,
	SEI,
	CLC,
	SEC,
	CLV,
	CLD,
	SED,
};

const TCHAR* ToString(TACOperator op);

// 操作数
// 与 NES 地址和寄存器相关
class TACOperand
{
public:
	// 操作数枚举
	enum OperandKind
	{
		INTEGER = 0,  // 整数常量, 值是8位整数值
		ADDRESS = 0x01000000,  // 内存地址，值是16位地址
		// 变量类别
		TEMP = 0x03000000,  // 临时变量，值是它的编号
		GLOBAL = 0x04000000,  // 全局变量，值是它的16位地址
		REGISTER = 0x05000000,  // 寄存器，值是寄存器编号

		KIND_MASK = 0xFF000000,
		VALUE_MASK = 0x0000FFFF,
	};

public:
	TACOperand();
	TACOperand(uint32_t value);
	bool operator==(const TACOperand& other) const;
	// 是否临时变量
	inline bool IsTemp() const { return GetKind() == TEMP; }
	// 是否寄存器
	inline bool IsRegister() const { return GetKind() == REGISTER; }
	// 是否内存地址
	inline bool IsAddress() const { return GetKind() == ADDRESS; }
	// 是否整数
	inline bool IsInterger() const { return GetKind() == INTEGER; }
	// 是否是变量
	inline bool IsVariable() const { return GetKind() >= TEMP && GetKind() <= REGISTER; }
	// 获取值
	inline uint32_t GetValue() const { return data & VALUE_MASK; }
	// 设置值
	inline void SetValue(int value) { data = (value & VALUE_MASK) | (data & ~VALUE_MASK); }
	// 获取类别
	inline uint32_t GetKind() const { return data & KIND_MASK; }
	// 设置类别
	inline void SetKind(uint32_t kind) { data = (data & ~KIND_MASK) | kind; }
	// 是否是零
	inline bool IsZero() const { return data == 0; }

	// 获取哈希值
	inline uint32_t GetHash() const { return data; }
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
	TAC(TACOperator op = TACOperator::NOP);
	TAC(TACOperator op, TACOperand z);
	TAC(TACOperator op, TACOperand z, TACOperand x);
	TAC(TACOperator op, TACOperand z, TACOperand x, TACOperand y);

	// 是否条件跳转
	inline bool IsConditionalJump() const { return op >= TACOperator::IFGEQ && op <= TACOperator::IFEQ; };
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




