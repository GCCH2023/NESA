#pragma once
#include "NesObject.h"

enum BasicBlockFlag
{
	BBF_END_NORMAL = 0,  // ����ָͨ�������˳��ת�����������
	BBF_END_RETURN = 1,  // �Է���ָ�����
	BBF_END_COND = 2,  // ��������תָ�����
	BBF_END_UNCOND = 3,  // ����������תָ�����
	BBF_END_MASK = 3,  // ����������־������

	// ����תָ�����ʱ�ı�־
	BBF_JUMP_BEFOER = 4,  // ������ת���͵�ַ��û�д˱�־������ת���ߵ�ַ

	BBF_JUMP_SELF = 8,  // ��ת���Լ�

	BBF_ENTRY = 16,  // ������ڻ�����
};

// �������һЩ����
// 1. ����Լ����Լ���ǰ������ô���Լ�����һ��ѭ��
// 2. һ�������������������̿飺
//    (1) ���û�к�̣���˵���Ǻ�������
//    (2) �����1����̣���������������תָ�����
//    (3) �����������̣�������������תָ�����
// 3. һ���������������ǰ��ת���͵�ַ������ܿ�����ѭ����䣻
//  ������ת��ض��Ƿ�֧���
class NesBasicBlock : public NesRegion
{
public:
	NesBasicBlock();
	NesBasicBlock(Nes::Address startAddr, Nes::Address endAddr);
	NesBasicBlock(const NesBasicBlock& other);

	void AddPrev(NesBasicBlock* block);
	// ��ȡ��̵�����
	size_t GetNextCount() const;
	// ��ȡǰ��������
	size_t GetPrevCount() const { return prevs.size(); }
	// �����Ϣ
	void Dump();
public:
	std::list<NesBasicBlock*> prevs; // ǰ����
	NesBasicBlock* nexts[2];  // ��̿�
	uint32_t flag;  // һЩ��־
};

