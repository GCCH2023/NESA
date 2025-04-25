#pragma once
#include "CBasicBlockBaseTranslator.h".h"
#include "TAC.h"

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
// 使用DAG来优化生成的C代码，可以去除临时变量和复用公共子表达式
// 注意:
// 1. 保留有副作用的语句，比如函数调用，数组赋值
// 2. 生成C代码时一定要缓存赋值过的变量，否则 x = x + 1; y = x * 2; 这样的的情况会有问题
class CBasicBlockDAGTranslator:
	public CBasicBlockBaseTranslator
{
	using DefinitionVar = std::unordered_map<CNode*, const Variable*>;
public:
	CBasicBlockDAGTranslator(CTranslator* translator);
	CNode* GetCondition() { return condition; }
	uint32_t GetJumpTarget() { return jumpAddr; }
protected:
	CNode* OnTranslate(const TACBasicBlock* block) override;
	CNode* TranslateTAC(const TAC* tac, size_t& index) override;
	CNode* BinAssignStatement(CNodeKind kind, const TAC* tac) override;
	CNode* AssignStatement(const TACOperand& z, CNode* x) override;
	CNode* FieldExpression(CNode* obj, const Field* field) override;
	CNode* IndexExpression(CNode* array, CNode* index) override;
	CNode* ArrayAssign(CNode* z, CNode* x) override;
	CNode* UnaryAssignStatement(CNodeKind kind, const TAC* tac) override;

	CNode* GetExpression(const TACOperand& operand) override;
	CNode* TranslateCall(const TAC* call, CNode* params) override;
	// 条件跳转语句翻译
	void ConditionalJump(const TAC* tac);

	// 获取节点表中的指定节点，不存在则添加
	CNode* GetNode(CNode* node);
	// 将变量附加到节点上
	void Attach(const TACOperand& var, CNode* node);
	// 构建DAG
	void GenerateDAG(const TACBasicBlock* block);
	// 从 DAG 生成 C 代码
	CNode* GenerateCodes();
	// 从 DAG 节点生成 AST 的表达式
	CNode* GenerateExpression(CNode* node, const DefinitionVar& definition);
	// 标记表达式需要被保留
	inline void Reserve(CNode* expr) { reserved.push_back(expr); }
	inline void Reserve(TACOperand operand) { reserved.push_back(operand); }
	// 标记三地址码是否被保留
	void MarkReserve(const TAC* tac);
	// 处理跳转指令
	void GenerateConditionalJump(const DefinitionVar& definition);
private:
	CNode* condition;  // 跳转指令对应的条件表达式
	uint32_t jumpAddr;  // 跳转指令对应的
	const TAC* jumpTAC = nullptr;  // 基本块末尾的跳转指令

	// 已存在的节点表
	std::unordered_set<CNode*, CNodeHash, CNodeEqual> nodeSet;
	// 变量到最近的赋值节点的映射
	std::unordered_map<TACOperand, CNode*, TACOperandHash> varMap;
	// 需要保留的表达式
	std::vector<std::variant<TACOperand, CNode*>> reserved;
};

