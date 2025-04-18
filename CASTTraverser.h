#pragma once

struct CNode;

// C 抽象语法树节点的访问器
class CASTVisitor
{
public:
	CASTVisitor() = default;
	virtual ~CASTVisitor() = default;

	// 前序遍历方法
	virtual void PreVisit(CNode* node) {}
	// 后序遍历方法
	virtual void PostVisit(CNode* node) {}
};

// C 抽象语法树的遍历器
class CASTTraverser
{
public:
	CASTTraverser() = default;
	virtual ~CASTTraverser() = default;
    // 开始遍历抽象语法树
    virtual void Traverse(CNode* node, CASTVisitor& visitor);

};