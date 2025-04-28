#pragma once
#include "CNode.h"

// 将指定节点修改为其他类别的节点
// 不会修改节点的链接关系
class NodeConverter
{
public:
	// 将 node 修改为 target
	static inline void To(Statement* node, Statement&& target)
	{
		*node = std::move(target);
	}
	static inline void To(Expression* node, Expression&& target)
	{
		*node = std::move(target);
	}
	// 将 node 修改为空语句节点
	static inline void ToEmptyStatement(Statement* node)
	{
		assert(node);

		*node = Statement::Empty();
	}
	// 将 node 修改为标签语句节点
	static inline void ToLabelStatement(Statement* node, String* label, Statement* body)
	{
		assert(node);

		*node = Statement::Label(label, body);
	}
	// 对表达式进行取反
	// 会修改输入的表达式的类型
	static Expression* Not(Expression* expr);
};

