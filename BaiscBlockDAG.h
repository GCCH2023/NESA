#pragma once
#include "CNode.h"
#include "TACFunction.h"

class Allocator;
class TACBasicBlock;
class CDataBase;

struct CNodeHash
{
	std::size_t operator()(const CNode* node) const;
};

// �Զ���ȽϺ���
struct CNodeEqual
{
	bool operator()(const CNode* node1, const CNode* node2) const;
};

struct TACOperandHash
{
	std::size_t operator()(const TACOperand& operand) const;
};


// �ڽ�����ַ�뷭��ΪC����ʱ��ʹ��DAG��
// �Ż���䣬����Ҫ��������ʱ����
class BaiscBlockDAG
{
public:
	BaiscBlockDAG(Allocator& allocator);
	~BaiscBlockDAG();
	CNode* Translate(TACBasicBlock* block);

protected:
	String* NewString(const CStr format, ...);
	CNode* GetExpression(TACOperand& operand);
	// ��ȡ�ڵ���е�ָ���ڵ㣬�����������
	CNode* GetNode(CNode* node);
	CNode* ConditionalJump(CNode*& condition, CNodeKind kind, TAC* tac, uint32_t& jumpAddr);
	CNode* NewNoneStatement();
	CNode* NewStatementList(CNode* head, CNode* tail);

protected:
	Allocator& allocator;

	std::unordered_set<CNode*, CNodeHash, CNodeEqual> nodeSet;  // �ڵ��ϣ�����ٲ��ҽڵ�
	std::unordered_map<TACOperand, CNode*, TACOperandHash> lastest;  // ��������Ķ�ֵ���

	CNode registers[5];  // AXYPSP 5���Ĵ���
};

