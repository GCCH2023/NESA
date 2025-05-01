#include "stdafx.h"
#include "CASTListOptimizer.h"
#include "NodeConverter.h"

void CASTListOptimizer::Reset()
{
	visited.clear();
}

//#include "Dump.h"
// 后序抽象语法树，有多条子语句则顺序遍历它们
void CASTListOptimizer::OnVisit(Statement* node)
{
	if (!node) return;

	// 首先处理子节点
	VisitChildren(node);

	if (!node->IsCompound())
		return;

	// 遍历列表语句的所有子节点
	//static int count = 0;
	//++count;
	//COUT << count << _T(" 优化列表前:\n");// << node << std::endl;
	//DumpCNodeStructures(COUT, node, 0);
	//COUT << std::endl;
	//if (count == 2)
	//{
	//	int a = 0;
	//}
	auto& list = node->AsList();
	for (auto it = list.begin(); it != list.end();)
	{
		if (it->IsEmpty())
		{
			it = list.erase(it);  // 删除空语句
			continue;
		}
		if (it->IsCompound())
		{
			//COUT << _T("修改前:\n");// << node << std::endl;
			//DumpCNodeStructures(COUT, node, 0);
			//COUT << std::endl;
			auto& child = it->AsList();
			//COUT << _T("子列表:\n");// << *pos << std::endl;
			//DumpCNodeStructures(COUT, *it, 0);
			//COUT << std::endl;
			list.insert(it, std::move(child));  // { } 的子节点上移一层
			// list.erase(*pos);  // 删除 {}
			//COUT << _T("修改后:\n");// << node << std::endl;
			//DumpCNodeStructures(COUT, node, 0);
			//COUT << std::endl;
			//COUT << _T("修改后:\n");// << node << std::endl;
			//DumpCNodeStructures(COUT, node, 0);
			//COUT << std::endl;
			//int a = 0;
			it = list.erase(it);  // 删除空列表
		}
		else
			++it;
	}

	switch (list.size())
	{
	case 0:  // 没有元素则修改为空语句节点
		NodeConverter::ToEmptyStatement(node);
		return;
	case 1:  // 只有一条子语句，去除列表
		NodeConverter::To(node, std::move(*list.front()));
		return;
	}

	//COUT << count << _T(" 优化列表后:\n");// << node << std::endl;
	//DumpCNodeStructures(COUT, node, 0);
	//COUT << std::endl;
}

void CASTListOptimizer::OnVisit(Expression* node)
{
	// 不需要遍历表达式节点
}
