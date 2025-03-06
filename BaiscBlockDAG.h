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

// 自定义比较函数
struct CNodeEqual
{
	bool operator()(const CNode* node1, const CNode* node2) const;
};

struct TACOperandHash
{
	std::size_t operator()(const TACOperand& operand) const;
};


// 在将三地址码翻译为C语句的时候，使用DAG来
// 优化语句，最主要是舍弃临时变量
class BaiscBlockDAG
{
public:
	BaiscBlockDAG(Allocator& allocator);
	~BaiscBlockDAG();
	CNode* Translate(TACBasicBlock* block);

protected:
	String* NewString(const CStr format, ...);
	CNode* GetExpression(TACOperand& operand);
	// 获取节点表中的指定节点，不存在则添加
	CNode* GetNode(CNode* node);
	CNode* ConditionalJump(CNode*& condition, CNodeKind kind, TAC* tac, uint32_t& jumpAddr);
	CNode* NewNoneStatement();
	CNode* NewStatementList(CNode* head, CNode* tail);

protected:
	Allocator& allocator;

	std::unordered_set<CNode*, CNodeHash, CNodeEqual> nodeSet;  // 节点哈希表，快速查找节点
	std::unordered_map<TACOperand, CNode*, TACOperandHash> lastest;  // 变量最近的定值语句

	CNode registers[5];  // AXYPSP 5个寄存器
};

