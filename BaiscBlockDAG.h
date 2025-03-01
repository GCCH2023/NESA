#pragma once
class Allocator;
struct CNode;
struct TACBasicBlock;

// �ڽ�����ַ�뷭��ΪC����ʱ��ʹ��DAG��
// �Ż����
class BaiscBlockDAG
{
public:
	BaiscBlockDAG(Allocator& allocator);
	~BaiscBlockDAG();
	CNode* Translate(TACBasicBlock* block);

protected:
	CStr NewString(const CStr format, ...);
	CNode* GetExpression(TACOperand& operand);
protected:
	Allocator& allocator;
};

