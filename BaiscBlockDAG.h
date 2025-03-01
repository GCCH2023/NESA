#pragma once
class Allocator;
struct CNode;
struct TACBasicBlock;

// 在将三地址码翻译为C语句的时候，使用DAG来
// 优化语句
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

