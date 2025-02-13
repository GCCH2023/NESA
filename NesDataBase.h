#pragma once
#include "Cartridge.h"
#include "Instruction.h"
#include "NesSubroutine.h"
#include "NesBasicBlock.h"

using InstructionList = std::vector<Instruction>;
using SubroutineList = std::vector<NesSubroutine*>;

// NES ���ݿ�
class NesDataBase
{
public:
	NesDataBase(const TCHAR* rom);
	~NesDataBase();

	// ��ȡ RESET �жϴ������ĵ�ַ
	inline Nes::Address GetInterruptResetAddress() const { return reset; }
	// ��ȡ NMI �жϴ������ĵ�ַ
	inline Nes::Address GetInterruptNmiAddress() const { return nmi; }
	// ��ȡ IRQ �жϴ������ĵ�ַ
	inline Nes::Address GetInterruptIrqAddress() const { return irq; }

	// ��ȡ��������
	inline Cartridge& GetCartridge() { return cartridge; }

	// ���ݵ�ַ��ȡָ��
	const Instruction* GetInstruction(Nes::Address address);
	// ��ȡָ����ַ��Χ�ڵ�ָ��
	void GetInstructions(InstructionList& list, Nes::Address begin, Nes::Address end);

	// �������ӻ�����
	void AddBasicBlock(NesBasicBlock* block);
	// ���ݵ�ַ���һ����飬�û��������ָ����ַ
	NesBasicBlock* FindBasicBlock(Nes::Address address);
	// ��ȡָ����ַ�����ڵĻ������б�
	BasicBlockList GetBasicBlocks(Nes::Address start, Nes::Address end);

	// ���������ӳ���
	void AddSubroutine(NesSubroutine* subroutine);
	// ���ݵ�ַ�����ӳ���
	NesSubroutine* FindSubroutine(Nes::Address address);
	// ��ȡ�ӳ����б�
	SubroutineList& GetSubroutines() { return subroutines; }

	// ���������ӳ�����ù�ϵ
	void AddCallRelation(CallRelation* call);

	Allocator allocator;

private:
	// ����
	Cartridge cartridge;

	// �����ж������Ĵ�������ַ
	Nes::Address nmi;
	Nes::Address reset;
	Nes::Address irq;

	BasicBlockList basicBlocks;  // �����������ַ��С��������
	CallList calls;  // �������ù�ϵ

	SubroutineList subroutines;  // �ӳ����б�
	std::vector<Instruction*> instructions;  // ָ���
};

