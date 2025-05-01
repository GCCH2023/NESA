#pragma once
#include "CBasicBlockBaseTranslator.h"
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
class CBasicBlockDAGTranslator :
	public CBasicBlockBaseTranslator
{
	using DefinitionVar = std::unordered_map<CNode*, const Variable*>;
public:
	CBasicBlockDAGTranslator(CTranslator* translator);
protected:
	Statement* OnTranslate(const TACBasicBlock* block) override;
	// 翻译单目表达式
	Expression* UnaryExpression(CNodeKind kind, Expression* x);
	// 翻译双目表达式
	Expression* BinaryExpression(CNodeKind kind, Expression* x, Expression* y);
	// 翻译赋值表达式 z = x
	Expression* AssignExpression(const TACOperand& z, Expression* x);
	// 翻译数组元素赋值或对象字段赋值语句
	Expression* ArrayAssign(Expression* z, Expression* x);
	// 翻译函数调用表达式 result = func(args)，需要判断是否接收返回值
	Expression* CallExpression(String* func, ConstArgList& args, const TACOperand& result);
	Expression* GetExpression(const TACOperand& operand) override;
	Expression* FieldExpression(Expression* obj, const Field* field) override;
	Expression* IndexExpression(Expression* array, Expression* index) override;
	// 翻译类型转换表达式
	Expression* CastExpression(const Type* type, Expression* value) override;
	// 处理条件跳转
	void ConditionalJump(CNodeKind op, Expression* x, Expression* y, uint32_t jump) override;

	// 获取节点表中的指定节点，不存在则添加
	Expression* GetNode(Expression&& node);
	Statement* GetNode(Statement&& node);

	// 将变量附加到节点上
	void Attach(const TACOperand& var, CNode* node);
	// 构建DAG
	void GenerateDAG(const TACBasicBlock* block);
	// 从 DAG 生成 C 代码
	Statement* GenerateCodes();
	// 从 DAG 节点生成 AST 的表达式
	Expression* GenerateExpression(CNode* node, const DefinitionVar& definition);
	// 标记表达式需要被保留
	inline void Reserve(CNode* var) { reserved.emplace_back(var); }
	inline void Reserve(TACOperand var, CNode* value) { reserved.emplace_back(TACVar{ var, value }); }
	// 处理跳转指令
	void GenerateConditionalJump(const DefinitionVar& definition);
private:
	// 已存在的节点表
	std::unordered_set<CNode*, CNodeHash, CNodeEqual> nodeSet;
	// 变量到最近的赋值节点的映射
	std::unordered_map<TACOperand, CNode*, TACOperandHash> varMap;

	// 变量与它对应的C节点
	using TACVar = std::pair<TACOperand, CNode*>;  // 变量, 值
	using CVar = CNode*;  // 数组，字段赋值，函数调用等有副作用的节点
	// 需要保留的表达式
	std::vector<std::variant<TACVar, CVar>> reserved;
};

