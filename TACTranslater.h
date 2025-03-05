#pragma once
#include "TAC.h"
#include "NesDataBase.h"

// �� NES ����ת��Ϊ NES ����ַ��
// TAC �ӳ��򣬻����飬ָ��ڷ������Ϸ���
class TACTranslater
{
public:
	TACTranslater(NesDataBase& db, Allocator& allocator);
	~TACTranslater();
	// �� NES �ӳ���ת��Ϊ ����ַ���ʾ���ӳ���
	TACSubroutine* Translate(NesSubroutine* subroutine);
	// �������ݣ����Ը��ö���
	void Reset();

protected:
	TACBasicBlock* TranslateBasickBlock(NesBasicBlock* block);
	// ��ȡָ��Ĳ�����
	// ���ڸ��ӵ�Ѱַģʽ�����ܻ����ö��������
	TACOperand GetOperand(const Instruction& instruction);
	// �����������������Ƿ���������
	bool TranslateOperand(TAC& tac, const Instruction& instruction);
	// ���� CMP, CPX, CPY ָ��
	// reg ָʾ���ĸ��Ĵ���
	TAC* TranslateCmp(const Instruction& instruction, const Instruction& next, TACOperand reg);
	// ���һ������ַ��
	void AddTAC(TAC* tac, Nes::Address address);
	// ��¼����ַ��Ŀ�ʼ����
	void SaveTACStart();
	// ���ݻ�������NESָ�����������ȡ��Ӧ������ַ��
	TAC* GetBlockTAC(int index);
	// ����������תָ��
	TAC* TranslateConditionalJump(std::vector<Instruction>& instructions, size_t index, uint32_t need, TACOperator op);
private:
	NesDataBase& db;
	Allocator& allocator;
	NesSubroutine* nesSub;
	TACSubroutine* tacSub;
	TACBasicBlock* tacBlock;
	// �ڽ�һ���������NESָ���Ϊ����ַ��Ĺ����У�
	// ��¼��ÿ��ָ���Ӧ������ַ��Ŀ�ʼ����
	std::vector<int> tacStarts;
};

