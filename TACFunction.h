#pragma once
#include "TAC.h"

using TACList = std::vector<TAC*>;
struct Type;

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
class TACFunction : public NesRegion
{
public:
	TACFunction();
	TACFunction(Nes::Address startAddress, Nes::Address endAddress);

	// �������
	void Dump();

	// ��ȡ��������ַ��
	TACList GetCodes();

	// ���һ��������
	inline void AddBasicBlock(TACBasicBlock* block) { blocks.push_back(block); }
	inline TACBasicBlockList& GetBasicBlocks() { return blocks; }

	// ���һ���µ���ʱ��������������
	// bytes ����ռ�õ��ֽ��� (Ŀǰû�õ���
	int NewTemp(Type* type);

	// ��ȡAXY������־
	uint32_t GetParamFlag() const { return flag & 7; }
	// ��ȡAXY����ֵ��־
	uint32_t GetReturnFlag() const { return (flag >> 3) & 7; }

	uint32_t flag;  // ��3λ��ʾAXY��Ϊ������3 - 5λ��ʾAXY��Ϊ����ֵ
private:
	TACBasicBlockList blocks;  // �������б�
	std::vector<Type*> tempTypes;  // ��ʱ����������
};

// ����ӳ����AXY���������
void DumpTACSubroutineAXY(TACFunction* sub);