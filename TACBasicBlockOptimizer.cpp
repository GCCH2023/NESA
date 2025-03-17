#include "stdafx.h"
#include "TACBasicBlockOptimizer.h"
#include "TAC.h"


std::size_t TACNodeHash::operator()(const TACNode* node) const
{
	if (node->op == TACOperator::NOP)
		return node->leaf;
	return (size_t)node->op ^ (size_t)node->x ^ (size_t)node;
}

bool TACNodeEqual::operator()(const TACNode* node1, const TACNode* node2) const
{
	if (node1->op != node2->op)
		return false;
	if (node1->op == TACOperator::NOP)
		return node1->leaf == node2->leaf;
	return node1->x == node2->x && node1->y == node2->y;
}


TACBasicBlockOptimizer::TACBasicBlockOptimizer(Allocator& allocator_) :
allocator(allocator_),
tempAllocator(1024 * 1024)
{
	tempCount = 100;
}

TACBasicBlockOptimizer::~TACBasicBlockOptimizer()
{
}


TACNode* TACBasicBlockOptimizer::GetNode(const TACOperand& operand)
{
	TACNode node;
	switch (operand.GetKind())
	{
	//case TACOperand::INTEGER:
	//	node.op = TACOperator::NOP;
	//	node.leaf = operand.ToInteger();
	//	break;
	//case TACOperand::ADDRESS:
	//	break;
	//case TACOperand::GLOBAL:
	//	break;
	case TACOperand::TEMP:
	case TACOperand::REGISTER:
	{
								 auto it = varMap.find(operand.ToInteger());
								 if (it != varMap.end())
									 return it->second;
								 break;
	}
	}
	node.op = TACOperator::NOP;
	node.leaf = operand.ToInteger();
	return GetNode(&node);
}


TACNode* TACBasicBlockOptimizer::GetNode(TACNode* node)
{
	// 找得到就返回
	auto it = tacNodeSet.find(node);
	if (it != tacNodeSet.end())
		return *it;
	// 找不到就添加后返回
	auto n = tempAllocator.New<TACNode>();
	n->op = node->op;
	if (node->op == TACOperator::NOP)
	{
		n->leaf = node->leaf;
	}
	else
	{
		n->x = node->x;
		n->y = node->y;
	}
	tacNodeSet.insert(n);
	return n;
}

TACOperand TACBasicBlockOptimizer::GenerateTAC(std::vector<TAC*>& codes, TACNode* node)
{
	if (node->op == TACOperator::NOP)
	{
		// 可能有变量附加到叶子节点，那么生成赋值指令
		for (auto v : node->attach)
		{
			auto tac = allocator.New<TAC>(TACOperator::ASSIGN);
			tac->z = v;
			tac->x = TACOperand(node->leaf);
			codes.push_back(tac);
		}
		node->attach.clear();  // 移除所有变量，避免重复生成
		return TACOperand(node->leaf);
	}

	if (!node->result.IsInterger())
		return node->result;  // 已经生成过这个节点对应的三地址码，则直接返回

	auto tac = allocator.New<TAC>(node->op);
	tac->x = GenerateTAC(codes, node->x);
	tac->y = GenerateTAC(codes, node->y);
	// 内部节点可能没有附加变量，这时要创建临时变量
	if (node->attach.empty())
	{
		tac->z = NewTemp();
		return tac->z;
	}

	tac->z = *node->attach.begin();
	codes.push_back(tac);
	node->result = tac->z;

	// 其他变量生成赋值指令
	for (auto v : node->attach)
	{
		if (v == tac->z)
			continue;

		auto code = allocator.New<TAC>(TACOperator::ASSIGN);
		code->z = v;
		code->x = tac->z;
		codes.push_back(code);
	}
	return tac->z;
}

void TACBasicBlockOptimizer::Attach(TACOperand& var, TACNode* node)
{
	// 首先将变量从原来附加的节点移除
	auto it = varMap.find(var.ToInteger());
	if (it != varMap.end())
	{
		it->second->attach.remove(var);
	}
	// 接着链接新节点
	varMap[var.ToInteger()] = node;
	node->attach.push_back(var);
}

TACOperand TACBasicBlockOptimizer::NewTemp()
{
	return TACOperand(TACOperand::TEMP | tempCount++);
}

std::vector<TAC*>& TACBasicBlockOptimizer::Optimize(std::vector<TAC*>& codes)
{
	TACNode node;
	// 遍历三地址码，生成DAG
	for (auto tac : codes)
	{
		switch (tac->op)
		{
		case TACOperator::ASSIGN:
			// 设置 z 的最新赋值节点
			Attach(tac->z, GetNode(tac->x));
			break;
		default:
			node.op = tac->op;
			node.x = GetNode(tac->x);
			node.y = GetNode(tac->y);
			// 设置 z 的最新赋值节点
			Attach(tac->z, GetNode(&node));
			break;
		}
	}
	// 遍历DAG，重新生成代码
	for (auto it : varMap)
	{
		auto node = it.second;
		GenerateTAC(result, node);
	}
	return result;
}
