#pragma once
#include "NodeSet.h"
#include "CNode.h"
#include "TAC.h"

class TACSubroutine;
class NesDataBase;
class Function;

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

class CTranslater
{
public:
	CTranslater(Allocator& allocator, NesDataBase& db);
	~CTranslater();

	// �����ӳ���ΪC����
	Function* TranslateSubroutine(TACSubroutine* subroutine);

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
	void OnReduceSelfLoop(ControlTreeNodeEx* node);
	void OnReduceList(ControlTreeNodeEx* node);
	void OnReducePoint2Loop(ControlTreeNodeEx* node);
	void OnReduceIf(ControlTreeNodeEx* node);
	void OnReduceIfElse(ControlTreeNodeEx* node);
	void OnReduceIfOr(ControlTreeNodeEx* node);
protected:
	// ������ַ�������ת��ΪC���ʽ
	CNode* GetExpression(TACOperand& operand);
	// ������ת��䷭��
	CNode* ConditionalJump(CNode*& condition, CNodeKind kind, TAC* tac, uint32_t& jumpAddr);
	// �����������Ϊ�����﷨���ڵ�
	CNode* TranslateRegion(CNode*& condition, TACBasicBlock* tacBlock, uint32_t& jumpAddr);
	// ������ת��ַ��ȡ��Ӧ�ı�ǩ���
	// CLabelStatement* GetLabel(uint32_t jumpAddr);
	// ��ȡ��ǩ����
	CStr GetLabelName(uint32_t jumpAddr);
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
	CStr NewString(const CStr format, ...);
	// ����һ��do while �ڵ�
	CNode* NewDoWhile(CNode* condition, CNode* body);
protected:
	NesDataBase& db;
	Allocator& allocator;  // ���ڴ���������
	Function* function;
	TACSubroutine* subroutine;

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

	CNode* registers[5];  // AXYPSP 5���Ĵ���
	CNode* noneStatement;  // �����
	std::unordered_map<Nes::Address, CStr> labels;  // ��ַ����ǩ����ӳ��
	//std::vector<Nes::Address> labels;
	std::unordered_map<Nes::Address, CNode*> blockStatements;  // ��ַ���������Ӧ������ӳ��
};

