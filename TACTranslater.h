#pragma once
#include "TAC.h"
#include "NesDataBase.h"

// �� NES ����ת��Ϊ NES ����ַ��
class TACTranslater
{
public:
	TACTranslater(NesDataBase& db, Allocator& allocator);
	~TACTranslater();
	// �� NES �ӳ���ת��Ϊ ����ַ���ʾ���ӳ���
	TACSubroutine* Translate(NesSubroutine* subroutine);
protected:
	TACBasicBlock* TranslateBasickBlock(NesBasicBlock* block);
	// ��ȡָ��Ĳ�����
	TACOperand GetOperand(const Instruction& instruction);
	// ���� CMP, CPX, CPY ָ��
	// reg ָʾ���ĸ��Ĵ���
	TAC* TranslateCmp(const Instruction& instruction, const Instruction& next, TACOperand reg);
	// ���һ������ַ��
	void AddTAC(TAC* tac);
private:
	NesDataBase& db;
	Allocator& allocator;
	NesSubroutine* nesSub;
	TACSubroutine* tacSub;
	TACBasicBlock* tacBlock;
};

