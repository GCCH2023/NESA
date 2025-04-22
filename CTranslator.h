#pragma once
#include "CNodeFactory.h"

class Function;
class TACFunction;
struct CNode;
class TACOperand;
struct Variable;
struct String;
struct Type;

// 三地址码翻译为C语句的基类
class CTranslator
{
public:
	CTranslator(Allocator& allocator);
	virtual ~CTranslator();

	// 翻译子程序为C函数
	Function* Translate(TACFunction* tacFunc);
	virtual void Reset();

	inline TACFunction* GetTACFunction() { return tacFunc; }
	inline Function* GetFunction() { return function; }
	// 将三地址码操作数转换为C表达式，节点动态分配内存
	CNode* GetExpression(const TACOperand& operand);
	// 将三地址码操作数转换为C表达式，节点由外部分配
	CNode* GetExpression(CNode& node, const TACOperand& operand);
	// 获取局部变量，不存在就添加
	const Variable* GetLocalVariable(String* name, Type* type);
	// 按索引获取局部变量
	const Variable* GetLocalVariable(int index);
	// 根据TAC中的临时变量索引获取C局部变量名称
	String* GetLocalVariableName(int index);
	// 创建C函数的类型
	void SetFunctionType();
	// 添加所有临时变量
	void SetLocalVariables();
	// 翻译函数体
	virtual CNode* TranslateBody();
	// 获取标签名称
	String* GetLabelName(uint32_t jumpAddr);
	// 回填标签语句
	void PatchLabels();
	// 添加地址语句映射
	void AddAddressMapStatement(uint32_t address, CNode* statement);
	// 创建一条列表语句
	CNode* NewStatementList(CNode* head, CNode* tail);
	// 获取寄存器的名称
	String* GetRegisterName(int index) { return registers[index]; }
	// 删除函数中没有用到的变量
	void RemoveUnusedLocalVariables();

	Allocator& GetAllocator() { return allocator; }
	CNodeFactory& GetNodeFactory() { return nodeFactory; }
protected:
	Allocator& allocator;
	CNodeFactory nodeFactory;
private:
	TACFunction* tacFunc;
	Function* function;
	String* registers[9];  // AXY NVZC P SP 9个寄存器
	std::unordered_map<Nes::Address, String*> labels;  // 地址到标签语句的映射
	std::unordered_map<Nes::Address, CNode*> blockStatements;  // 地址到基本块对应的语句的映射
};

