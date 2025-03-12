#pragma once
#include "NesObject.h"


// ������
enum class TACOperator
{
	NOP,  // ռλ
	ADD,  // �ӷ�
	SUB,  // ����
	ASSIGN, // ��ֵ z = x
	BOR,  // λ�� |, z = x | y
	BAND,  // λ�� &, z = x & y
	BNOT,  // ȡ�� z = ~x
	XOR, // ��� z = x ^ y
	IFGREAT,  // if x > y goto z
	IFGEQ,  // if x >= y goto z
	IFLESS,  // if x < y goto z
	IFLEQ,  // if x <= y goto z
	IFNEQ,  // if x != y goto z
	IFEQ,  // if x == y goto z
	ARRAY_GET,  // ��ȡ����Ԫ�� z = x[y]��x ���������ַ��y ��Ԫ�ص�ƫ�����ֽ���
	ARRAY_SET,  // ��������Ԫ�� x[y] = z��x ���������ַ��y ��Ԫ�ص�ƫ�����ֽ���
	ADDR,  // ȡ��ַ��z = &x
	DEREF,  // �����ã��� z ��T���ͣ���x ��Tָ�����ͣ�z = *x
	CAST,  // ����ת��, z = (T)x��T�ɷ��ű��е�z���͸���
	ARG,  // ����һ�������������൱�� x86 �е� push x
	CALL, // z = x(y)��x �Ǻ�����ַ��y�ǲ�������
	GOTO, // goto z
	RETURN,  // return x
	SHR,  // �߼����ƣ�z = x >> y���������λ
	SHL,  // �߼����ƣ�z = x << y���������λ
	ROR,  // ѭ�����ƣ�z = x >> y�����λ��Ϊ���λ
	ROL,  // ѭ�����ƣ�z = x << y�����λ��Ϊ���λ
	PUSH, // x ��ջ
	POP,  // ��ջ�� z

	// �� 6502 ��صĲ�����
	BREAK,  // brk
	BIT,  // bit������֪������ô���루��֪�����ָ����÷���
	CLI,
	SEI,
	CLC,
	SEC,
	CLV,
	CLD,
	SED,
};

const TCHAR* ToString(TACOperator op);

// ������
// �� NES ��ַ�ͼĴ������
class TACOperand
{
public:
	// ������ö��
	enum OperandKind
	{
		INTEGER = 0,  // ��������, ֵ��8λ����ֵ
		ADDRESS = 0x01000000,  // �ڴ��ַ��ֵ��16λ��ַ
		// �������
		TEMP = 0x03000000,  // ��ʱ������ֵ�����ı��
		GLOBAL = 0x04000000,  // ȫ�ֱ�����ֵ������16λ��ַ
		REGISTER = 0x05000000,  // �Ĵ�����ֵ�ǼĴ������

		KIND_MASK = 0xFF000000,
		VALUE_MASK = 0x0000FFFF,
	};

public:
	TACOperand();
	TACOperand(uint32_t value);
	bool operator==(const TACOperand& other) const;
	// �Ƿ���ʱ����
	inline bool IsTemp() const { return GetKind() == TEMP; }
	// �Ƿ�Ĵ���
	inline bool IsRegister() const { return GetKind() == REGISTER; }
	// �Ƿ��ڴ��ַ
	inline bool IsAddress() const { return GetKind() == ADDRESS; }
	// �Ƿ�����
	inline bool IsInterger() const { return GetKind() == INTEGER; }
	// �Ƿ��Ǳ���
	inline bool IsVariable() const { return GetKind() >= TEMP && GetKind() <= REGISTER; }
	// ��ȡֵ
	inline uint32_t GetValue() const { return data & VALUE_MASK; }
	// ����ֵ
	inline void SetValue(int value) { data = (value & VALUE_MASK) | (data & ~VALUE_MASK); }
	// ��ȡ���
	inline uint32_t GetKind() const { return data & KIND_MASK; }
	// �������
	inline void SetKind(uint32_t kind) { data = (data & ~KIND_MASK) | kind; }
	// �Ƿ�����
	inline bool IsZero() const { return data == 0; }

	// ��ȡ��ϣֵ
	inline uint32_t GetHash() const { return data; }
private:
	uint32_t data;
};

OStream& operator<<(OStream& os, const TACOperand& obj);

// NES �Ĵ��� ������
extern TACOperand RegisterP;
extern TACOperand RegisterA;
extern TACOperand RegisterX;
extern TACOperand RegisterY;
extern TACOperand RegisterSP;

// ����ַ��
// ����һ�������������������
// x �� y ��Ϊ�����������z ��Ϊ���
class TAC
{
public:
	TAC(TACOperator op = TACOperator::NOP);
	TAC(TACOperator op, TACOperand z);
	TAC(TACOperator op, TACOperand z, TACOperand x);
	TAC(TACOperator op, TACOperand z, TACOperand x, TACOperand y);

	// �Ƿ�������ת
	inline bool IsConditionalJump() const { return op >= TACOperator::IFGEQ && op <= TACOperator::IFEQ; };
public:
	uint32_t address;
	TACOperator op;
	TACOperand x;
	TACOperand y;
	TACOperand z;
};

// ���㳣����������ֵ
// ������ǳ����׳��쳣
// ����ǵ�Ŀ������Ļ���y��ֵӦ��������0
TACOperand Evaluate(TACOperator op, TACOperand x, TACOperand y);
OStream& operator<<(OStream& os, const TAC* obj);
// �������ַ������ַ��
OStream& DumpAddressTAC(OStream& os, const TAC* tac);




