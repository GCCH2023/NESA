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
	// ��ͬ���Ͷ�Ӧ��ͬ���ֶ�
	union
	{
		// ��ѭ��������
		ControlTreeNodeEx* node;
		// ������������ �� 2��ѭ��
		struct
		{
			ControlTreeNodeEx* first;
			ControlTreeNodeEx* second;
		} pair;
		// if ����
		struct
		{
			ControlTreeNodeEx* condition;
			ControlTreeNodeEx* then;
			ControlTreeNodeEx* _else;
		} _if;
	};
	CNode* statement;  // ��Ӧ��C��䣬���һ��������ֻ��һ����ת��䣬��ô����ֶ�Ϊ��
	// ��Ӧ���������ʽ������˳������鹹�ɵ�ѭ������Ȼ���ȹ�Լ������У�������ѭ������ô��Ҫ����
	// �����������������ʽ
	CNode* condition;
};

// �������鹹�ɵĿ�����ͼ����ΪC���
// ������Ҫע����ǣ�������ͼ�еı��޷���ʾ�����ǰ��˳��
// ���� a -> b, b -> a, ���Թ�ԼΪ ����ѭ�������Ǵ�ָ������
// ��Ȼֻ��һ�֣�����������ֻ������һ���ر�
class CTranslater
{
public:
	CTranslater(Allocator& allocator, NesDataBase& db);
	~CTranslater();

	// �����ӳ���ΪC����
	Function* TranslateSubroutine(TACFunction* subroutine);

protected:
	// ���������ͼ�ı߼����������ÿ���������������ڵ�
	ControlTreeNodeEx* Analyze(Edge edges[], size_t count);
	// ���ݱ߼�����������ͼ
	void BuildCFG(Edge edges[], size_t count);
	// �����ڲ�����
	void Reset();
	// �����ɽڵ��ԼΪһ���ڵ㣬����������ڵ��C���
	Node CReduce(Node parent, std::vector<Node> children, CtrlTreeNodeType type);
	// ����һ���µĻ�����
	Node CreateBasicBlock();
	// ����һ���µĿ������ڵ�
	Node CreateControlTreeNode();
	// ��Լ�������򹹳ɵ���������  a -> b
	Node ReduceRegionList(NodeSet& N, Node a, Node b);
	// ��Լ��ѭ�� a -> a
	Node ReduceRegionSelfLoop(NodeSet& N, Node a);
	// ��Լ if else �ṹ�� a -> b, a -> c, b -> d, c -> d
	// a �� if ���������ڻ����飬b �� c �����������Ͳ���������ִ�еĻ����飬d �� if ִ�к󵽴�Ļ�����
	Node ReduceRegionIfElse(NodeSet& N, Node a, Node b, Node c);
	// ��Լ if else �ṹ�� a -> b, a -> c, b -> d, c -> d ����չ
	// ���һ���� b -> c
	// a �� if ���������ڻ����飬b �� c �����������Ͳ���������ִ�еĻ����飬d �� if ִ�к󵽴�Ļ�����
	Node ReduceRegionIfOr(NodeSet& N, Node a, Node b, Node c);
	// ��Լ if �ṹ�� a -> b, a -> c, b -> c
	// a �� if ���������ڻ����飬b ��������ִ�еĻ����飬c �� if ִ�к󵽴�Ļ�����
	Node ReduceRegionIf(NodeSet& N, Node a, Node b);
	// a->b, b->a �� b ֻ��һ��ǰ��
	// a ���� b֮��ĺ�̱ߣ�����Ϊgoto���
	Node ReduceRegionPoint2Loop(NodeSet& N, Node a, Node b);


	// ��ȡ�������ڵ�����Ҷ�ӽڵ㼯��
	void GetLeafEntry(ControlTreeNodeEx* node, std::vector<Node>& nodes);
	// ��ȡ����ĳ���Ҷ�ӽڵ�
	void GetLeafExit(ControlTreeNodeEx* node, std::vector<Node>& nodes);
	// ��ȡ����ߣ��߼��ṹ�еıߣ���Ӧ��Ҷ�ӱ�
	// һ������߶�Ӧ����Ҷ�ӱ�
	std::vector<Edge> GetLeafEdges(Edge e);

	// ����������ͼ�ڵ㼯����ȡ�������ڵ㼯
	NodeSet CAnalysis(NodeSet N);
	// ��ȡ���нڵ㹹�ɵļ���
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
	// ��ȡȫ�ֱ����������ھ����
	const Variable* GetGlobalVariable(CAddress address, Type* type);
	// ��ȡ�ֲ������������ھ����
	const Variable* GetLocalVariable(String* name, Type* type);
	// ��������ȡ�ֲ�����
	const Variable* GetLocalVariable(int index);
	// ������ַ�������ת��ΪC���ʽ
	CNode* GetExpression(TACOperand& operand);
	// ������ת��䷭��
	CNode* ConditionalJump(CNode*& condition, CNodeKind kind, TAC* tac, uint32_t& jumpAddr);
	// ��������ַ��Ĳ�����ΪC���Եı��ʽ����
	CNodeKind TranslateOperator(TACOperator op);
	// �����������Ϊ�����﷨���ڵ�
	CNode* TranslateRegion(CNode*& condition, TACBasicBlock* tacBlock, uint32_t& jumpAddr);
	// ������ת��ַ��ȡ��Ӧ�ı�ǩ���
	// CLabelStatement* GetLabel(uint32_t jumpAddr);
	// ��ȡ��ǩ����
	String* GetLabelName(uint32_t jumpAddr);
	// ������֧����������
	// Ҳ����������к������һ��if���
	// һ��������ͨ��ǰ����˳��ִ�е�ָ������������תָ���β
	// statement �� if ǰ�����䣬����Ϊ�գ���ʾ���������ֻ��һ��������תָ��
	// condition��if��������body��if����Ϊ��Ҫִ�е����, elseBody ������Ϊ��Ҫִ�е����
	CNode* CombineListIf(CNode* statement, CNode* condition, CNode* body, CNode* elseBody = nullptr);
	// �Ա��ʽ����ȡ��
	// ���ܻ��޸�����ı��ʽ������
	CNode* GetNotExpression(CNode* expr);
	// �����ǩ���
	void PatchLabels();
	// ����һ���ַ���
	String* NewString(const CStr format, ...);
	// ����һ��do while �ڵ�
	CNode* NewDoWhile(CNode* condition, CNode* body);
	// ����һ���б����ڵ�
	CNode* NewStatementList(CNode* head, CNode* tail);
	// ����һ��ֻ��������������б�ڵ�
	CNode* NewStatementPair(CNode* first, CNode* second);
	// ����һ�������
	CNode* NewNoneStatement();
protected:  // C��������
	// ����C����������
	void SetFunctionType();
	// ���������ʱ����
	void SetLocalVariables();
protected:
	NesDataBase& db;
	Allocator& allocator;  // ���ڴ���������
	Function* function;
	TACFunction* subroutine;
protected:
	// ����ʹ��
	// ������л����鹹�ɵĿ�����ͼ
	void DumpCFG();
	// ������п������ڵ㹹�ɵĿ�����ͼ
	void DumpControlTree();
	// �����ǰ�����Ľڵ㼯
	void DumpCurrentCFG(NodeSet& N);
private:
	Allocator tempAllocator;  // ���ڴ�����ʱ�ڵ�
	BasicBlock blocks[MAX_NODE];  // �������б�ÿ���������Ӧ������ͼ�е�һ���ڵ�
	int blockCount;

	ControlTreeNodeEx* ctrees[MAX_NODE];  // �������ڵ��б�
	int controlTreeNodeCount;

	String* registers[5];  // AXYPSP 5���Ĵ���
	std::unordered_map<Nes::Address, String*> labels;  // ��ַ����ǩ����ӳ��
	//std::vector<Nes::Address> labels;
	std::unordered_map<Nes::Address, CNode*> blockStatements;  // ��ַ���������Ӧ������ӳ��
};

