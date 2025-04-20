#pragma once
#include "CNode.h"
#include "TAC.h"

class TACBasicBlock;
class TACOperand;
class CTranslator;

struct CNodeHash
{
	std::size_t operator()(const CNode* node) const;
};

// 自定义比较函数
struct CNodeEqual
{
	bool operator()(const CNode* node1, const CNode* node2) const;
};

// 将三地址码基本块翻译为C代码
// 跳转指令不会被翻译C语句，而是保存为条件表达式和跳转目标地址
// 使用DAG来优化生成的C代码
class CBasicBlockDAGTranslator
{
public:
	CBasicBlockDAGTranslator(CTranslator* translator);
	CNode* Translate(TACBasicBlock* block);
	CNode* GetCondition() { return condition; }
	uint32_t GetJumpTarget() { return jumpAddr; }
protected:
	CNode* GetExpression(const TACOperand& operand);
	const Variable* GetVariable(const TACOperand& operand);
	CNode* TranslateCall(TAC* call, CNode* params);
	// 条件跳转语句翻译
	CNode* ConditionalJump(CNodeKind kind, TAC* tac, uint32_t& jumpAddr);

	// 获取节点表中的指定节点，不存在则添加
	CNode* GetNode(CNode* node);
	// 根据参数构造CNode对象，并返回节点表中的对应对象
	template<typename... Args>
	CNode* GenNode(Args&&... args)
	{
		CNode node(std::forward<Args>(args)...);
		return GetNode(&node);
	}
	// 将变量附加到节点上
	void Attach(TACOperand& var, CNode* node);
	// 构建DAG
	void GenerateDAG(TACBasicBlock* block);
	// 从 DAG 生成 C 代码
	CNode* GenerateCodes();
	// 从 DAG 节点生成 AST 的表达式
	CNode* GenerateExpression(CNode* node);

private:
	Allocator& allocator;
	CTranslator* translator;
	CNode* condition;  // 跳转指令对应的条件表达式
	uint32_t jumpAddr;  // 跳转指令对应的

	// 已存在的节点表
	std::unordered_set<CNode*, CNodeHash, CNodeEqual> nodeSet;
	// 变量到最近的赋值节点的映射
	std::unordered_map<TACOperand, CNode*, TACOperandHash> varMap;
	// 需要保留的表达式

};

