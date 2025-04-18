#pragma once

struct CNode;
class CASTContextTraverser;

// C 抽象语法树节点的访问器
class CASTContextVisitor
{
public:
	CASTContextVisitor() = default;
	virtual ~CASTContextVisitor() = default;

	// 设置遍历器
	void SetTraverser(CASTContextTraverser* traverser) { this->traverser = traverser; }
	CASTContextTraverser* GetTraverser() { return traverser; }

	// 前序遍历方法
	virtual void PreVisit(CNode* node, int depth) {}
	// 后序遍历方法
	virtual void PostVisit(CNode* node, int depth) {}
private:
	CASTContextTraverser* traverser = nullptr;
};

struct CNodeContext
{
	CNode* node;
	int depth;
};

using CNodeContextList = std::vector<CNodeContext>;

// 保存遍历过程的祖先节点和兄姊节点的遍历器
// 兄姊栈中也保存着祖先的兄姊节点，需要根据深度来判断是不是当前节点的
class CASTContextTraverser
{
public:
	CASTContextTraverser() :
		ancestors(16),
		seniors(16)
	{
	}
	virtual ~CASTContextTraverser() = default;
	// 开始遍历抽象语法树
	virtual void Traverse(CNode* node, CASTContextVisitor& visitor);
	CNodeContextList& GetAncestors() { return ancestors; }
	CNodeContextList& GetSeniors() { return seniors; }
protected:
	// 遍历节点并保存信息
	void TraverseNode(CNode* node, CASTContextVisitor& visitor);
	// 重置数据
	void Reset();
	void PopSeniors();
	void PushSenior(CNode* node) { seniors.push_back({ node, depth }); }
	void PushAncestor(CNode* node) { ancestors.push_back({ node, depth }); ++depth; }
	void PopAncestor() { PopSeniors(); ancestors.pop_back(); --depth;}
private:
	CNodeContextList ancestors;  // 祖先栈
	CNodeContextList seniors;  // 兄姊栈
	int depth;  // 当前遍历的节点的嵌套深度
};