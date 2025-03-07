#pragma once
#include "NesObject.h"

class NesBasicBlock;

using BasicBlockList = std::vector<NesBasicBlock*>;

// �ӳ���ĵ��ù�ϵ
struct CallRelation
{
	Nes::Address call;  // ���÷��ӳ����ַ
	Nes::Address called;  // �������ӳ����ַ

	CallRelation();
	CallRelation(Nes::Address call, Nes::Address called);
	inline Nes::Address GetStartAddress() const { return call; }
	inline void SetStartAddress(Nes::Address addr) { call = addr; }
};

using CallList = std::vector<CallRelation*>;

enum SubroutineFlag
{
	SUBF_PARAM_A = 1,  // ʹ�� A �Ĵ�����Ϊ����
	SUBF_PARAM_X = 2,  // ʹ�� X �Ĵ�����Ϊ����
	SUBF_PARAM_Y = 4,  // ʹ�� Y �Ĵ�����Ϊ����
	SUBF_PARAM = 7,  // ʹ���� AXY �Ĵ�����Ϊ����
	SUBF_RETURN_A = 8,  // ʹ�� A �Ĵ�����Ϊ����ֵ
	SUBF_RETURN_X = 0x10,  // ʹ�� X �Ĵ�����Ϊ����ֵ
	SUBF_RETURN_Y = 0x20,  // ʹ�� Y �Ĵ�����Ϊ����ֵ
	SUBF_RETURN = 0x38,  // ʹ���� AXY �Ĵ�����Ϊ����ֵ
	SUBF_LOCAL_A = 0x40,  // ʹ�� A �Ĵ�����Ϊ�ֲ�����
	SUBF_LOCAL_X = 0x80,  // ʹ�� X �Ĵ�����Ϊ�ֲ�����
	SUBF_LOCAL_Y = 0x100,  // ʹ�� Y �Ĵ�����Ϊ�ֲ�����
	SUBF_LOCAL = 0x1C0,  // ʹ���� AXY �Ĵ�����Ϊ�ֲ�����
};

class NesSubroutine : public NesRegion
{
public:
	NesSubroutine();
	NesSubroutine(Nes::Address startAddr, Nes::Address endAddr);

	// ����ַ��С�������ӻ����飬�ظ���ַ�����
	void AddBasicBlock(NesBasicBlock* block);
	const BasicBlockList& GetBasicBlocks() const { return blocks; }
	// ���ݵ�ַ���һ�����
	NesBasicBlock* FindBasicBlock(Nes::Address addr);

	// ��ȡ���õ��ӳ����б�
	inline std::vector<Nes::Address>& GetCalls() { return calls; }
	// ���һ�������ӳ����ַ
	void AddCall(Nes::Address addr);

	// ����ӳ������Ϣ
	void Dump();

	// ��ȡAXY������־
	uint32_t GetParamFlag() const { return flag & 7; }
	// ��ȡAXY����ֵ��־
	uint32_t GetReturnFlag() const { return (flag >> 3) & 7; }

	uint32_t flag;  // ��3
protected:
	BasicBlockList blocks;  // �ӳ�������Ļ������б�����ַ��С��������
	std::vector<Nes::Address> calls;  // ���õ��ӳ����ַ�����Ϊ�գ�����ӳ���û�е��������ӳ��򣬴�С��������
};

