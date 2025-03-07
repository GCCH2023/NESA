#pragma once
#include "NodeSet.h"
#include "CNode.h"
#include "TACFunction.h"

class TACFunction;
class NesDataBase;
class Function;
struct Type;
class CDataBase;
struct String;

struct ControlTreeNodeEx : BasicBlock
{
	CtrlTreeNodeType type;
	// 不同类型对应不同的字段
	union
	{
		// 自循环的区域
		ControlTreeNodeEx* node;
		// 连续两个区域 或 2点循环
		struct
		{
			ControlTreeNodeEx* first;
			ControlTreeNodeEx* second;
		} pair;
		// if 区域
		struct
		{
			ControlTreeNodeEx* condition;
			ControlTreeNodeEx* then;
			ControlTreeNodeEx* _else;
		} _if;
	};
	CNode* statement;  // 对应的C语句，如果一个基本块只有一条跳转语句，那么这个字段为空
	// 对应的条件表达式，两个顺序基本块构成的循环，必然是先归约语句序列，而后是循环，那么就要保存
	// 后面基本块的条件表达式
	CNode* condition;
};

// 将基本块构成的控制流图翻译为C语句
// 但是需要注意的是：控制流图中的边无法表示代码的前后顺序
// 比如 a -> b, b -> a, 可以归约为 两种循环，但是从指令来看
// 必然只有一种，这两条边中只可能有一条回边
class CTranslater
{
public:
	CTranslater(Allocator& allocator, NesDataBase& db);
	~CTranslater();

	// 翻译子程序为C代码
	Function* TranslateSubroutine(TACFunction* subroutine);

protected:
	// 输入控制流图的边集，分析后获得控制树，返回其根节点
	ControlTreeNodeEx* Analyze(Edge edges[], size_t count);
	// 根据边集构建控制流图
	void BuildCFG(Edge edges[], size_t count);
	// 重置内部数据
	void Reset();
	// 将若干节点归约为一个节点，并生成这个节点的C语句
	Node CReduce(Node parent, std::vector<Node> children, CtrlTreeNodeType type);
	// 创建一个新的基本块
	Node CreateBasicBlock();
	// 创建一个新的控制树节点
	Node CreateControlTreeNode();
	// 归约两个区域构成的连续区域  a -> b
	Node ReduceRegionList(NodeSet& N, Node a, Node b);
	// 归约自循环 a -> a
	Node ReduceRegionSelfLoop(NodeSet& N, Node a);
	// 归约 if else 结构体 a -> b, a -> c, b -> d, c -> d
	// a 是 if 的条件所在基本块，b 和 c 是满足条件和不满足条件执行的基本块，d 是 if 执行后到达的基本块
	Node ReduceRegionIfElse(NodeSet& N, Node a, Node b, Node c);
	// 归约 if else 结构体 a -> b, a -> c, b -> d, c -> d 的扩展
	// 多出一条边 b -> c
	// a 是 if 的条件所在基本块，b 和 c 是满足条件和不满足条件执行的基本块，d 是 if 执行后到达的基本块
	Node ReduceRegionIfOr(NodeSet& N, Node a, Node b, Node c);
	// 归约 if 结构体 a -> b, a -> c, b -> c
	// a 是 if 的条件所在基本块，b 满足条件执行的基本块，c 是 if 执行后到达的基本块
	Node ReduceRegionIf(NodeSet& N, Node a, Node b);
	// a->b, b->a 且 b 只有一个前驱
	// a 除了 b之外的后继边，翻译为goto语句
	Node ReduceRegionPoint2Loop(NodeSet& N, Node a, Node b);


	// 获取控制树节点的入口叶子节点集合
	void GetLeafEntry(ControlTreeNodeEx* node, std::vector<Node>& nodes);
	// 获取区域的出口叶子节点
	void GetLeafExit(ControlTreeNodeEx* node, std::vector<Node>& nodes);
	// 获取抽象边（高级结构中的边）对应的叶子边
	// 一条抽象边对应多条叶子边
	std::vector<Edge> GetLeafEdges(Edge e);

	// 分析控制流图节点集，获取控制树节点集
	NodeSet CAnalysis(NodeSet N);
	// 获取所有节点构成的集合
	NodeSet GetFullSet()
	{
		return (1 << blockCount) - 1;
	}
	void InitializeBaseControlTree(NodeSet N);

protected:
	void OnReduceSelfLoop(Node node);
	void OnReduceList(Node first, Node second);
	void OnReducePoint2Loop(Node first, Node second);
	void OnReduceIf(Node _if, Node then);
	void OnReduceIfElse(Node _if, Node then, Node _else);
	void OnReduceIfOr(Node _if, Node then, Node _else);
protected:
	// 获取全局变量，不存在就添加
	const Variable* GetGlobalVariable(CAddress address, Type* type);
	// 获取局部变量，不存在就添加
	const Variable* GetLocalVariable(String* name, Type* type);
	// 按索引获取局部变量
	const Variable* GetLocalVariable(int index);
	// 将三地址码操作数转换为C表达式
	CNode* GetExpression(TACOperand& operand);
	// 条件跳转语句翻译
	CNode* ConditionalJump(CNode*& condition, CNodeKind kind, TAC* tac, uint32_t& jumpAddr);
	// 翻译三地址码的操作码为C语言的表达式类型
	CNodeKind TranslateOperator(TACOperator op);
	// 翻译区域代码为抽象语法树节点
	CNode* TranslateRegion(CNode*& condition, TACBasicBlock* tacBlock, uint32_t& jumpAddr);
	// 根据跳转地址获取对应的标签语句
	// CLabelStatement* GetLabel(uint32_t jumpAddr);
	// 获取标签名称
	String* GetLabelName(uint32_t jumpAddr);
	// 创建分支基本块的语句
	// 也就是语句序列后面跟着一个if语句
	// 一个基本块通常前面是顺序执行的指令，最后以条件跳转指令结尾
	// statement 是 if 前面的语句，可以为空，表示这个基本块只有一条条件跳转指令
	// condition是if的条件，body是if条件为真要执行的语句, elseBody 是条件为假要执行的语句
	CNode* CombineListIf(CNode* statement, CNode* condition, CNode* body, CNode* elseBody = nullptr);
	// 对表达式进行取反
	// 可能会修改输入的表达式的类型
	CNode* GetNotExpression(CNode* expr);
	// 回填标签语句
	void PatchLabels();
	// 创建一个字符串
	String* NewString(const CStr format, ...);
	// 创建一个do while 节点
	CNode* NewDoWhile(CNode* condition, CNode* body);
	// 创建一个列表语句节点
	CNode* NewStatementList(CNode* head, CNode* tail);
	// 创建一个只有两条语句的语句列表节点
	CNode* NewStatementPair(CNode* first, CNode* second);
	// 创建一个空语句
	CNode* NewNoneStatement();
protected:  // C函数处理
	// 创建C函数的类型
	void SetFunctionType();
	// 添加所有临时变量
	void SetLocalVariables();
protected:
	NesDataBase& db;
	Allocator& allocator;  // 用于创建输出结果
	Function* function;
	TACFunction* subroutine;
protected:
	// 调试使用
	// 输出所有基本块构成的控制流图
	void DumpCFG();
	// 输出所有控制树节点构成的控制流图
	void DumpControlTree();
	// 输出当前分析的节点集
	void DumpCurrentCFG(NodeSet& N);
private:
	Allocator tempAllocator;  // 用于创建临时节点
	BasicBlock blocks[MAX_NODE];  // 基本块列表，每个基本块对应控制流图中的一个节点
	int blockCount;

	ControlTreeNodeEx* ctrees[MAX_NODE];  // 控制树节点列表
	int controlTreeNodeCount;

	String* registers[5];  // AXYPSP 5个寄存器
	std::unordered_map<Nes::Address, String*> labels;  // 地址到标签语句的映射
	//std::vector<Nes::Address> labels;
	std::unordered_map<Nes::Address, CNode*> blockStatements;  // 地址到基本块对应的语句的映射
};

