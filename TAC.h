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
	INDEX,  // ���� z = x[y]
	REF,  // ������ x[y] = z
	ARG,  // ����һ�������������൱�� x86 �е� push x
	CALL, // z = x(y)��x �Ǻ�����ַ��y�ǲ�������
	GOTO, // goto z
	RETURN,  // return
	SHR,  // �߼����ƣ�z = x >> y���������λ
	SHL,  // �߼����ƣ�z = x << y���������λ
	ROR,  // ѭ�����ƣ�z = x >> y�����λ��Ϊ���λ
	ROL,  // ѭ�����ƣ�z = x << y�����λ��Ϊ���λ
	PUSH, // x ��ջ
	POP,  // ��ջ�� z

	BREAK,  // brk
	BIT,  // bit������֪������ô���루��֪�����ָ����÷���
};

const TCHAR* ToString(TACOperator op);

// ������
// �� NES ��ַ�ͼĴ������
class TACOperand
{
public:
	enum
	{
		INTEGER = 0,  // ��������, ֵ��8λ����ֵ
		ADDRESS = 0x01000000,  // �ڴ��ַ��ֵ��16λ��ַ
		MEMORY = 0x10000000,  // �ڴ浥Ԫ��ֵ��16λ��ַ
		REGISTER = 0x11000000,  // �Ĵ�����ֵ�ǼĴ������

		TEMP = 0x00100000,  // ��ʱ������ֵ�����ı��

		VALUE_MASK = 0xFFFF,
		KIND_MASK = 0xFF000000,
	};
	TACOperand();
	TACOperand(uint32_t value);
	bool operator==(const TACOperand& other);
	// �Ƿ���ʱ����
	inline bool IsTemp() const { return (data & TEMP) != 0; }
	// �Ƿ�Ĵ���
	inline bool IsRegister() const { return GetKind() == REGISTER; }
	// �Ƿ��ڴ��ַ
	inline bool IsAddress() const { return GetKind() == ADDRESS; }
	// �Ƿ�����
	inline bool IsInterger() const { return GetKind() == INTEGER; }
	// �Ƿ��ڴ浥Ԫ
	inline bool IsMemory() const { return GetKind() == MEMORY; }
	// ��ȡֵ
	inline uint32_t GetValue() const { return data & VALUE_MASK; }
	// ����ֵ
	inline void SetValue(int value) { data |= value & VALUE_MASK; }
	// ��ȡ���
	inline uint32_t GetKind() const { return data & KIND_MASK; }
	// �������
	inline void SetKind(uint32_t kind) { data = (data & ~KIND_MASK) | kind; }
	// �Ƿ�����
	inline bool IsZero() const { return data == 0; }
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

// ���㳣����������ֵ
// ������ǳ����׳��쳣
// ����ǵ�Ŀ������Ļ���y��ֵӦ��������0
TACOperand Evaluate(TACOperator op, TACOperand x, TACOperand y);
OStream& operator<<(OStream& os, const TAC* obj);
// �������ַ������ַ��
OStream& DumpAddressTAC(OStream& os, const TAC* tac);


using TACList = std::vector<TAC*>;

// ����ַ��Ļ�����
class TACBasicBlock : public NesRegion
{
public:
	TACBasicBlock();
	TACBasicBlock(Nes::Address startAddress, Nes::Address endAddress);

	// ���һ������ַ��
	inline void AddTAC(TAC* code) { codes.push_back(code); }
	inline TACList& GetCodes() { return codes; }
	// ��ȡ����ַ�������
	inline size_t GetCodesCount() const { return codes.size(); }
private:
	TACList codes;  // ����ַ���б�
public:
	std::vector<TACBasicBlock*> prevs;
	std::vector<TACBasicBlock*> nexts;
	uint32_t flag;  // һЩ��־
};

using TACBasicBlockList = std::vector<TACBasicBlock*>;

// ����ַ���ӳ���
class TACSubroutine : public NesRegion
{
public:
	TACSubroutine();
	TACSubroutine(Nes::Address startAddress, Nes::Address endAddress);

	// �������
	void Dump();

	// ��ȡ��������ַ��
	TACList GetCodes();

	// ���һ��������
	inline void AddBasicBlock(TACBasicBlock* block) { blocks.push_back(block); }
	inline TACBasicBlockList& GetBasicBlocks() { return blocks; }

	// ���һ���µ���ʱ��������������
	// bytes ����ռ�õ��ֽ��� (Ŀǰû�õ���
	inline int NewTemp(int bytes) { return tempCount++; }

	// ��ȡAXY������־
	uint32_t GetParamFlag() const { return flag & 7; }
	// ��ȡAXY����ֵ��־
	uint32_t GetReturnFlag() const { return (flag >> 3) & 7; }

	uint32_t flag;  // ��3λ��ʾAXY��Ϊ������3 - 5λ��ʾAXY��Ϊ����ֵ
private:
	TACBasicBlockList blocks;  // �������б�
	int tempCount;  // ��ʱ����������
};

// ����ӳ����AXY���������
void DumpTACSubroutineAXY(TACSubroutine* sub);

