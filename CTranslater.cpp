#include "stdafx.h"
#include "CTranslater.h"
using namespace std;

CTranslater::CTranslater(Allocator& allocator_, NesDataBase& db_) :
db(db_),
allocator(allocator_),
function(nullptr),
subroutine(nullptr),
tempAllocator(1024 * 1024)
{
	registers[Nes::NesRegisters::A] = allocator.New<CNode>(NewString(_T("A")));
	registers[Nes::NesRegisters::X] = allocator.New<CNode>(NewString(_T("X")));
	registers[Nes::NesRegisters::Y] = allocator.New<CNode>(NewString(_T("Y")));
	registers[Nes::NesRegisters::P] = allocator.New<CNode>(NewString(_T("P")));
	registers[Nes::NesRegisters::SP] = allocator.New<CNode>(NewString(_T("SP")));

	noneStatement = allocator.New<CNode>(CNodeKind::STAT_NONE);
}


CTranslater::~CTranslater()
{
}

Function* CTranslater::TranslateSubroutine(TACSubroutine* subroutine)
{
	if (!subroutine)
		return nullptr;
	this->subroutine = subroutine;

	// ���ȹ���߼�
	vector<Edge> edges(32);
	edges.clear();
	auto& blocks = subroutine->GetBasicBlocks();
	// ����������
	for (size_t i = 0; i < blocks.size(); ++i)
	{
		blocks[i]->tag = (void*)i;
	}
	for (auto block : blocks)
	{
		for (auto succ : block->nexts)
		{
			edges.push_back({ (int)block->tag, (int)succ->tag });
		}
	}
	auto root = Analyze(edges.data(), edges.size());
	// ����������������C���
	// COUT << root->statement;
	Function* func = allocator.New<Function>();
	func->SetBody(root->statement);
	TCHAR buffer[64];
	_stprintf_s(buffer, _T("sub_%04X"), subroutine->GetStartAddress());
	func->name = buffer;
	return func;
}

ControlTreeNodeEx* CTranslater::Analyze(Edge edges[], size_t count)
{
	BuildCFG(edges, count);
	NodeSet N = CAnalysis(GetFullSet());
	if (N.GetSize() != 1)  // Ҳ����ֻ��һ��������
	{
		DumpCFG();
		throw Exception(_T("�������޷���Լ����һ���ڵ�"));
	}
	// ��ȫ����䶼���ɺ󣬻����ǩ���
	PatchLabels();

	Node n = N.ToVector()[0];
	if (ctrees[n]->statement == nullptr)
	{
		CNode* condition = nullptr;
		uint32_t jumpAddr;
		auto block = this->subroutine->GetBasicBlocks()[n];
		ctrees[n]->statement = TranslateRegion(condition, block, jumpAddr);
	}
	return ctrees[n];
}

void CTranslater::BuildCFG(Edge edges[], size_t count)
{
	Reset();

	for (size_t i = 0; i < count; ++i)
	{
		Edge& p = edges[i];
		int first = p.from;
		int second = p.to;
		blocks[first].succ |= 1 << second;
		blocks[second].pred |= 1 << first;

		if (blockCount < first)
			blockCount = first;
		if (blockCount < second)
			blockCount = second;
	}
	++blockCount;
	for (int i = 0; i < blockCount; ++i)
	{
		blocks[i].index = i;
	}
}

void CTranslater::Reset()
{
	memset(blocks, 0, sizeof(blocks));
	memset(ctrees, 0, sizeof(ctrees));
	blockCount = 0;
	controlTreeNodeCount = 0;
	tempAllocator.Reset();
}

CNode* CTranslater::GetExpression(TACOperand& operand)
{
	switch (operand.GetKind())
	{
	case TACOperand::INTEGER:
		if (operand.IsTemp())
		{
			return allocator.New<CNode>(NewString(_T("temp%d"), operand.GetValue()), VAR_KIND_LOCAL);
		}
		else
			return allocator.New<CNode>(operand.GetValue());
	case TACOperand::REGISTER:
		return registers[operand.GetValue()];
	case TACOperand::MEMORY:
	{
							   if (operand.IsTemp())
							   {
								   auto var = allocator.New<CNode>(NewString(_T("temp%d"), operand.GetValue()), VAR_KIND_GLOBAL);
								   // ��Ҫ������
								   return allocator.New<CNode>(CNodeKind::EXPR_REF, var);
							   }
							   return allocator.New<CNode>(NewString(_T("g_%04X"), operand.GetValue()), VAR_KIND_GLOBAL);
	}
	case TACOperand::ADDRESS:
	{
								return allocator.New<CNode>(NewString(_T("g_%04X"), operand.GetValue()), VAR_KIND_GLOBAL);
	}
	default:
	{
			   TCHAR buffer[64];
			   _stprintf_s(buffer, _T("����ַ��תC��䣺δʵ�ֵ�����ַ�������ת��"));
			   throw Exception(buffer);
	}
	}
}


CNode* CTranslater::ConditionalJump(CNode*& condition, CNodeKind kind, TAC* tac, uint32_t& jumpAddr)
{
	// ������תָ��ض��ǻ��������ָ��
	condition = allocator.New<CNode>(kind, GetExpression(tac->x), GetExpression(tac->y));
	jumpAddr = tac->z.GetValue();
	return condition;
}


CNode* CTranslater::TranslateRegion(CNode*& pCondition, TACBasicBlock* tacBlock, uint32_t& jumpAddr)
{
	CNode* current = nullptr, *head = nullptr, *tail = nullptr;
	CNode* expr = nullptr;
	auto& codes = tacBlock->GetCodes();
	for (size_t i = 0; i < codes.size(); ++i)
	{
		auto tac = codes[i];
		switch (tac->op)
		{
		case	TACOperator::BOR:
			expr = allocator.New<CNode>(CNodeKind::EXPR_BOR, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case	TACOperator::BAND:
			expr = allocator.New<CNode>(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case	TACOperator::ASSIGN:
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), GetExpression(tac->x));
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case	TACOperator::ADD:
			expr = allocator.New<CNode>(CNodeKind::EXPR_ADD, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case	TACOperator::SUB:
			expr = allocator.New<CNode>(CNodeKind::EXPR_SUB, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case	TACOperator::XOR:
			expr = allocator.New<CNode>(CNodeKind::EXPR_XOR, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case TACOperator::SHL:
			expr = allocator.New<CNode>(CNodeKind::EXPR_SHIFT_LEFT, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case TACOperator::SHR:
			expr = allocator.New<CNode>(CNodeKind::EXPR_SHIFT_RIGHT, GetExpression(tac->x), GetExpression(tac->y));
			expr = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), expr);
			current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
			break;
		case	TACOperator::ARG:
		{
									// ���ɸ� ARG �������һ�� CALL
									// ���� ARG����Ҫ���ź����ֱ�� CALL ������ַ��һ����
									CNode* params = nullptr;
									CNode* paramsTail = nullptr;
									while (codes[i]->op == TACOperator::ARG)
									{
										if (!paramsTail)
										{
											paramsTail = params = GetExpression(codes[i]->x);
										}
										else
										{
											paramsTail->next = GetExpression(codes[i]->x);
											paramsTail = paramsTail->next;
										}
										++i;
									}
									if (codes[i]->op != TACOperator::CALL)
										throw Exception(_T("����ַ�뷭��ΪC��䣺ARG ���治�� CALL"));
									// ����� CALL ָ��
									auto call = allocator.New<CNode>(NewString(_T("sub_%04X"), codes[i]->x.GetValue()), params);
									current = call;
									break;
		}
		case	TACOperator::CALL:
		{
									 // ����в���������� ���ɸ� ARG �������һ�� CALL
									 // ֱ�ӳ��� CALL��˵��û�в���
									 current = allocator.New<CNode>(NewString(_T("sub_%04X"), tac->x.GetValue()), (CNode*)nullptr);
									 break;
		}

		case TACOperator::IFGEQ:  // ��תָ���ǻ���������һ��ָ��
			ConditionalJump(pCondition, CNodeKind::EXPR_GREAT_EQUAL, tac, jumpAddr);
			continue;
		case TACOperator::IFGREAT:
			ConditionalJump(pCondition, CNodeKind::EXPR_GREAT, tac, jumpAddr);
			continue;
		case TACOperator::IFEQ:
			ConditionalJump(pCondition, CNodeKind::EXPR_EQUAL, tac, jumpAddr);
			continue;
		case TACOperator::IFNEQ:
			ConditionalJump(pCondition, CNodeKind::EXPR_NOT_EQUAL, tac, jumpAddr);
			continue;
		case TACOperator::IFLESS:
			ConditionalJump(pCondition, CNodeKind::EXPR_LESS, tac, jumpAddr);
			continue;
		case TACOperator::IFLEQ:
			ConditionalJump(pCondition, CNodeKind::EXPR_LESS_EQUAL, tac, jumpAddr);
			continue;
		case TACOperator::GOTO:
		{
								  // goto �ڿ�����ͼ�ж�Ӧһ���ߣ����ܱ������ѭ���ṹ��Ҳ���ܾ��Ƕ�Ӧgoto��䣬
								  // ����֪������ô����
								  if (tac->z.GetValue() == tac->address)
								  {
									  // ��ת���Լ�����䷭��Ϊ while (1);
									  //expr = allocator.New<CInteger>(1);
									  //current = allocator.New<CWhileStatement>(expr, noneStatement);
									  current = noneStatement;
									  pCondition = allocator.New<CNode>(1);
									  break;
								  }
								  auto label = GetLabelName(tac->z.GetValue());
								  current = allocator.New<CNode>(CNodeKind::STAT_GOTO, label);
								  break;
		}
		case TACOperator::BIT:
		{
								 // ����������պ�һ�£�����ɱ��ʽ���
								 expr = allocator.New<CNode>(CNodeKind::EXPR_BAND, GetExpression(tac->x), GetExpression(tac->y));
								 current = allocator.New<CNode>(CNodeKind::STAT_EXPR, expr);
								 break;
		}
		case TACOperator::RETURN:
		{
									// ����ֵ�Ժ��ٿ��ǰ�
									current = allocator.New<CNode>(CNodeKind::STAT_RETURN);
									break;
		}
		case TACOperator::ROR:
		{
								 // C������û��ROR�����������Ϊ�������ú���
								 // void Ror(int*, int)
								 CNode* params = allocator.New<CNode>(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
								 params->next = GetExpression(tac->y);
								 current = allocator.New<CNode>(NewString(_T("Ror")), params);
								 break;
		}
		case TACOperator::ROL:
		{
								 // C������û��ROL�����������Ϊ�������ú���
								 // void Rol(int*, int)
								 CNode* params = allocator.New<CNode>(CNodeKind::EXPR_ADDR, GetExpression(tac->x));
								 params->next = GetExpression(tac->y);
								 current = allocator.New<CNode>(NewString(_T("Rol")), params);
								 break;
		}
		case TACOperator::PUSH:
		{
								  // ����֪����ô����push���ȷ���Ϊ�������ð�
								  CNode* params = GetExpression(tac->x);
								  current = allocator.New<CNode>(NewString(_T("Push")), params);
								  break;
		}
		case TACOperator::POP:
		{
								 // ����֪����ô����pop���ȷ���Ϊ�������ð�
								 current = allocator.New<CNode>(NewString(_T("Pop")), (CNode*)nullptr);
								 current = allocator.New<CNode>(CNodeKind::EXPR_ASSIGN, GetExpression(tac->z), current);
								 break;
		}

		case TACOperator::CLI:
		{
								 // ����Ϊ��������
								 current = allocator.New<CNode>(NewString(_T("Cli")), (CNode*)nullptr);
								 break;
		}
		case TACOperator::SEI:
		{
								 // ����Ϊ��������
								 current = allocator.New<CNode>(NewString(_T("Sei")), (CNode*)nullptr);
								 break;
		}
		case TACOperator::CLD:
		{
								 // ����Ϊ��������
								 current = allocator.New<CNode>(NewString(_T("Cld")), (CNode*)nullptr);
								 break;
		}
		case TACOperator::SED:
		{
								 // ����Ϊ��������
								 current = allocator.New<CNode>(NewString(_T("Sed")), (CNode*)nullptr);
								 break;
		}
		//case TACOperator::CLC:
		//case TACOperator::SEC:
		//case TACOperator::CLV:
			// ��λ�������־���Ǻ�����ָ�����ʹ�õģ������﷨���в�Ӧ�ó���
		default:
		{
				   TCHAR buffer[64];
				   _stprintf_s(buffer, _T("����ַ��תC��䣺δʵ�ֵ�����ַ�� %s"), ToString(tac->op));
				   throw Exception(buffer);
		}
		}
		// ��������б�
		if (!head)
		{
			head = tail = current;
		}
		else
		{
			tail->next = current;
			tail = current;
		}
	}
	Nes::Address firstAddr = codes.empty() ? tacBlock->GetStartAddress() : codes[0]->address;
	auto ret = NewStatementList(head, tail);  // ������һ��������ֻ��һ����תָ��ɣ����ؿ����
	blockStatements[firstAddr] = ret;  // ��¼������������Ӧ�ĵ�ַ�����
	return ret;
}

//CLabelStatement* CTranslater::GetLabel(uint32_t jumpAddr)
//{
//	auto it = labels.find(jumpAddr);
//	if (it == labels.end())
//	{
//		auto name = GetLabelName(jumpAddr);
//		CLabelStatement* label = allocator.New<CLabelStatement>(name.c_str());  // Ŀ��������
//		labels[jumpAddr] = label;
//		return label;
//	}
//	return it->second;
//}


CStr CTranslater::GetLabelName(uint32_t jumpAddr)
{
	auto it = labels.find(jumpAddr);
	if (it == labels.end())
	{
		auto label = NewString(_T("L%04X"), jumpAddr);
		labels[jumpAddr] = label;
		return label;
	}
	return it->second;
}


CNode* CTranslater::CombineListIf(CNode* statement, CNode* condition, CNode* body, CNode* elseBody /*= nullptr*/)
{
	auto ifStat = allocator.New<CNode>(CNodeKind::STAT_IF, condition, body, elseBody);
	if (!statement)
		return ifStat;
	return NewStatementPair(statement, ifStat);
}

CNode* CTranslater::GetNotExpression(CNode* expr)
{
	switch (expr->kind)
	{
	case CNodeKind::EXPR_GREAT:
		expr->kind = CNodeKind::EXPR_LESS_EQUAL;
		break;
	case CNodeKind::EXPR_GREAT_EQUAL:
		expr->kind = CNodeKind::EXPR_LESS;
		break;
	case CNodeKind::EXPR_LESS:
		expr->kind = CNodeKind::EXPR_GREAT_EQUAL;
		break;
	case CNodeKind::EXPR_LESS_EQUAL:
		expr->kind = CNodeKind::EXPR_GREAT;
		break;
	case CNodeKind::EXPR_EQUAL:
		expr->kind = CNodeKind::EXPR_NOT_EQUAL;
		break;
	case CNodeKind::EXPR_NOT_EQUAL:
		expr->kind = CNodeKind::EXPR_EQUAL;
		break;
	default:
		throw Exception(_T("δʵ�ֵı��ʽȡ������"));
	}
	return expr;
}

void CTranslater::PatchLabels()
{
	for (auto label : labels)
	{
		auto statement = blockStatements[label.first];
		if (statement->kind != CNodeKind::STAT_LABEL)
		{
			// �޸�Ϊ��ǩ���
			auto body = allocator.New<CNode>();
			*body = *statement;
			statement->kind = CNodeKind::STAT_LABEL;
			statement->l.body = body;
			statement->l.name = label.second;
		}
	}
}

CStr CTranslater::NewString(const CStr format, ...)
{
	TCHAR buffer[256];
	va_list va;
	va_start(va, format);
	_vstprintf_s(buffer, 256, format, va);
	va_end(va);
	size_t count = ((_tcslen(buffer) + 1) * sizeof(TCHAR)+sizeof(void*)-1) & ~(sizeof(void*)-1);
	CStr str = allocator.Alloc<TCHAR>(count);
	_tcscpy_s(str, count, buffer);
	return str;
}

CNode* CTranslater::NewDoWhile(CNode* condition, CNode* body)
{
	// do ; while (condition) => while (condition) ;
	if (body == noneStatement)
		return allocator.New<CNode>(CNodeKind::STAT_WHILE, condition, body);
	return allocator.New<CNode>(CNodeKind::STAT_DO_WHILE, condition, body);
}

CNode* CTranslater::NewStatementList(CNode* head, CNode* tail)
{
	if (!head)
		return noneStatement;
	if (head == tail)
		return head;
	return allocator.New<CNode>(CNodeKind::STAT_LIST, head, tail);
}

CNode* CTranslater::NewStatementPair(CNode* first, CNode* second)
{
	assert(first->next == nullptr);
	// �����Ż�
	// �������Ż���������һ�����⣺�еĵط���������������һ��ָ��
	// �ϲ��󣬱������ˣ�����ʧЧ��
	// �����������Ҳ���������⣬��Ϊ�����Լ��������һ�㲻������
	if (first->kind == CNodeKind::STAT_LIST)
	{
		if (second->kind == CNodeKind::STAT_LIST)
		{
			// �ϲ���ĩβ
			first->list.tail->next = second->list.head;
			first->list.tail = second->list.tail;
			return first;
		}
		// ��ӵ�ĩβ
		first->list.tail->next = second;
		first->list.tail = second;
		return first;
	}
	else if (second->kind == CNodeKind::STAT_LIST)
	{
		// ��ӵ���ͷ
		first->next = second->list.head;
		second->list.head = first;
		return second;
	}
	first->next = second;
	return NewStatementList(first, second);
}

// ��Ҫ��������ͼ�е�һ����ѭ���ڵ��Լʱ
// a -> a
void CTranslater::OnReduceSelfLoop(ControlTreeNodeEx* node)
{
	auto body = node->node;
	if (body->type != CTNTYPE_LEAF)
	{
		if (body->condition == nullptr)
			throw Exception(_T("��Ҷ����ѭ���ڵ��쳣"));
		node->statement = NewDoWhile(body->condition, body->statement);
		return;
	}
	CNode* condition = nullptr;
	auto block = this->subroutine->GetBasicBlocks()[body->index];
	uint32_t jumpAddr;
	auto a = TranslateRegion(condition, block, jumpAddr);
	node->statement = NewDoWhile(condition, a);
}

void CTranslater::OnReduceList(ControlTreeNodeEx* node)
{
	auto first = node->pair.first;
	auto second = node->pair.second;
	CNode* condition = nullptr;
	auto blocks = this->subroutine->GetBasicBlocks();
	uint32_t jumpAddr;
	if (first->type == CTNTYPE_LEAF)
	{
		first->statement = TranslateRegion(condition, blocks[first->index], jumpAddr);
	}
	if (second->type == CTNTYPE_LEAF)
	{
		second->statement = TranslateRegion(condition, blocks[second->index], jumpAddr);
	}
	node->statement = NewStatementPair(first->statement, second->statement);
	node->condition = condition;
}

void CTranslater::OnReducePoint2Loop(ControlTreeNodeEx* node)
{
	auto first = node->pair.first;
	auto second = node->pair.second;
	CNode* condition = nullptr;
	auto blocks = this->subroutine->GetBasicBlocks();
	uint32_t jumpAddr;
	if (first->type == CTNTYPE_LEAF)
	{
		first->statement = TranslateRegion(condition, blocks[first->index], jumpAddr);
		// ��ת�߷���Ϊ goto ���
		auto gotoStat = allocator.New<CNode>(CNodeKind::STAT_GOTO, GetLabelName(jumpAddr));
		first->statement = CombineListIf(first->statement, condition, gotoStat);
	}
	else
	{
		throw Exception(_T("2��ѭ���ĵ�һ���ڵ㲻��Ҷ�ӽڵ�Ĺ�Լδʵ��"));
	}
	if (second->type == CTNTYPE_LEAF)
	{
		second->statement = TranslateRegion(condition, blocks[second->index], jumpAddr);
	}
	else
	{
		assert(second->statement);
		assert(second->condition);
		condition = second->condition;
	}
	node->statement = NewStatementPair(first->statement, second->statement);
	node->statement = NewDoWhile(condition, node->statement);
}

void CTranslater::OnReduceIf(ControlTreeNodeEx* node)
{
	auto cond = node->_if.condition;
	auto body = node->_if.then;
	CNode* condition = nullptr;
	auto blocks = this->subroutine->GetBasicBlocks();
	uint32_t jumpAddr;
	if (cond->type == CTNTYPE_LEAF)
	{
		cond->statement = TranslateRegion(condition, blocks[cond->index], jumpAddr);
	}
	else
	{
		// �������Ҷ�ӽڵ㣬��֮ǰ�Ĺ�Լ��ȻҪ����������
		if (!cond->condition)
			throw Exception(_T("����Ϊ if ���Ĺ�����ȱ�� if �����������ʽ"));
		condition = cond->condition;
	}
	CNode* ifCond = condition;
	if (body->type == CTNTYPE_LEAF)
	{
		body->statement = TranslateRegion(condition, blocks[body->index], jumpAddr);
	}
	// �� if ���֮ǰ����һ�δ���
	node->statement = CombineListIf(cond->statement, ifCond, body->statement);
}

void CTranslater::OnReduceIfElse(ControlTreeNodeEx* node)
{
	auto cond = node->_if.condition;
	auto then = node->_if.then;
	auto _else = node->_if._else;
	CNode* condition = nullptr;
	auto blocks = this->subroutine->GetBasicBlocks();
	uint32_t jumpAddr;
	if (cond->type == CTNTYPE_LEAF)
	{
		cond->statement = TranslateRegion(condition, blocks[cond->index], jumpAddr);
	}
	else
	{
		// �������Ҷ�ӽڵ㣬��֮ǰ�Ĺ�Լ��ȻҪ����������
		if (!cond->condition)
			throw Exception(_T("����Ϊ if - else ���Ĺ�����ȱ�� if �����������ʽ"));
		condition = cond->condition;
	}
	CNode* ifCond = condition;
	// ��Ҫ������ת��ַ���ж��ĸ��������� then ���֣��ĸ��� else ����
	if (jumpAddr == blocks[then->index]->GetStartAddress())
	{
		// �����������Ҫ���� then �� else ����
		std::swap(node->_if.then, node->_if._else);
		std::swap(then, _else);
	}
	if (then->type == CTNTYPE_LEAF)
	{
		then->statement = TranslateRegion(condition, blocks[then->index], jumpAddr);
	}
	if (_else->type == CTNTYPE_LEAF)
	{
		_else->statement = TranslateRegion(condition, blocks[_else->index], jumpAddr);
	}
	// �� if ���֮ǰ����һ�δ���
	node->statement = CombineListIf(cond->statement, ifCond, then->statement, _else->statement);
}

// a -> b, a -> c, b ->c, b -> d, c -> d ����Ϊ if (x || y) { c }
// ���� a ���� ���� x��b �������� y�� c����������ʱҪִ�е�
void CTranslater::OnReduceIfOr(ControlTreeNodeEx* node)
{
	auto a = node->_if.condition;
	auto b = node->_if.then;
	auto c = node->_if._else;
	auto blocks = this->subroutine->GetBasicBlocks();
	uint32_t jumpAddr;
	CNode* condition1 = nullptr, *condition2 = nullptr;
	if (a->type == CTNTYPE_LEAF)
	{
		a->statement = TranslateRegion(condition1, blocks[a->index], jumpAddr);
	}
	else
	{
		// �������Ҷ�ӽڵ㣬��֮ǰ�Ĺ�Լ��ȻҪ����������
		if (!a->condition)
			throw Exception(_T("����Ϊ if - or ���Ĺ�����ȱ�� if ���ĵ�1���������ʽ"));
		condition1 = a->condition;
	}
	// ��Ҫ������ת��ַ���ж��ĸ��������� then ���֣��ĸ��� else ����
	if (jumpAddr == blocks[b->index]->GetStartAddress())
	{
		// �����������Ҫ���� then �� else ����
		std::swap(node->_if.then, node->_if._else);
		std::swap(b, c);
	}
	if (b->type == CTNTYPE_LEAF)
	{
		b->statement = TranslateRegion(condition2, blocks[b->index], jumpAddr);
	}
	else
	{
		// �������Ҷ�ӽڵ㣬��֮ǰ�Ĺ�Լ��ȻҪ����������
		if (!b->condition)
			throw Exception(_T("����Ϊ if ���Ĺ�����ȱ�� if ���ĵ�2���������ʽ"));
		condition2 = b->condition;
	}
	// �� || ���� a �� b ��������b������Ҫȡ������Ϊb��������ʱ��ת��d
	condition2 = GetNotExpression(condition2);
	condition1 = allocator.New<CNode>(CNodeKind::EXPR_OR, condition1, condition2);
	if (c->type == CTNTYPE_LEAF)
	{
		c->statement = TranslateRegion(condition2, blocks[c->index], jumpAddr);
	}

	// ����������߷���Ϊ goto ��䣬����ıߵ�β�ڵ�ֻ��һ����̣����Բ����condition��ֵ
	//auto name = GetLabelName(blocks[c->index]->GetStartAddress());
	//auto label = allocator.New<CLabelStatement>(name.c_str(), c->statement);  // β�ڵ������滻Ϊ��ǩ���
	//c->statement = label;

	//auto gotoStat = allocator.New<CGotoStatement>(label);
	//b->statement = CombineListIf(b->statement, condition, b->statement, c->statement);  // ͷ�ڵ��ĩβ����һ��������ת���

	// �� if ���֮ǰ����һ�δ���
	node->statement = CombineListIf(a->statement, condition1, c->statement);
}

Node CTranslater::CReduce(Node parent, vector<Node> children, CtrlTreeNodeType type)
{
	ControlTreeNodeEx* ctNode = ctrees[parent];
	ctNode->type = type;
	ctNode->index = parent;
	switch (type)
	{
	case CTNTYPE_LEAF:
		throw Exception(_T("���ܽ������ԼΪҶ������"));
		break;
	case CTNTYPE_LIST:
		ctNode->pair.first = ctrees[children[0]];
		ctNode->pair.second = ctrees[children[1]];
		OnReduceList(ctNode);
		break;
	case CTNTYPE_P2LOOP:
		ctNode->pair.first = ctrees[children[0]];
		ctNode->pair.second = ctrees[children[1]];
		OnReducePoint2Loop(ctNode);
		break;
	case CTNTYPE_SELF_LOOP:
		ctNode->node = ctrees[children[0]];
		OnReduceSelfLoop(ctNode);
		break;
	case CTNTYPE_IF:
		ctNode->_if.condition = ctrees[children[0]];
		ctNode->_if.then = ctrees[children[1]];
		OnReduceIf(ctNode);
		break;
	case CTNTYPE_IF_ELSE:
		ctNode->_if.condition = ctrees[children[0]];
		ctNode->_if.then = ctrees[children[1]];
		ctNode->_if._else = ctrees[children[2]];
		OnReduceIfElse(ctNode);
		break;
	case CTNTYPE_IF_OR:
		ctNode->_if.condition = ctrees[children[0]];
		ctNode->_if.then = ctrees[children[1]];
		ctNode->_if._else = ctrees[children[2]];
		OnReduceIfOr(ctNode);
		break;
	}
	//COUT << "��Լ " << ToString(type) << " " << parent << " : ";
	//for (auto n : children)
	//	COUT << n << ", ";
	//COUT << endl;
	//ctNode->Dump();
	//COUT << ctNode->statement;
	//COUT << endl;
	return parent;
}

Node CTranslater::CreateBasicBlock()
{
	if (blockCount >= MAX_NODE)
		throw Exception(_T("��������������"));
	blocks[blockCount].index = blockCount;
	return blockCount++;
}

Node CTranslater::CreateControlTreeNode()
{
	if (controlTreeNodeCount >= MAX_NODE)
		throw Exception(_T("�������ڵ���������"));
	ctrees[controlTreeNodeCount] = tempAllocator.New<ControlTreeNodeEx>();
	ctrees[controlTreeNodeCount]->index = controlTreeNodeCount;
	return controlTreeNodeCount++;
}

Node CTranslater::ReduceRegionList(NodeSet& N, Node a, Node b)
{
	Node r = CreateControlTreeNode();
	// r ��ǰ���� a ��ǰ��
	ctrees[r]->pred = ctrees[a]->pred;
	for (auto pred : ctrees[a]->Pred())
		ctrees[pred]->succ.Replace(a, r);

	// r �ĺ���� b �ĺ��
	ctrees[r]->succ = ctrees[b]->succ;
	for (auto s : ctrees[b]->Succ())
		ctrees[s]->pred.Replace(b, r);

	// ʹ�� r ���� a, b
	N.Replace({ a, b }, { r });
	return CReduce(r, { a, b }, CTNTYPE_LIST);
}

Node CTranslater::ReduceRegionSelfLoop(NodeSet& N, Node a)
{
	Node r = CreateControlTreeNode();
	// r ��ǰ���� a ���� a ֮���ǰ��
	ctrees[r]->pred = ctrees[a]->pred - a;
	for (auto pred : ctrees[r]->Pred())
		ctrees[pred]->succ.Replace(a, r);

	// r �ĺ���� a ���� a ֮��ĺ��
	ctrees[r]->succ = ctrees[a]->succ - a;
	for (auto s : ctrees[r]->Succ())
		ctrees[s]->pred.Replace(a, r);

	// ʹ�� r ���� a
	N.Replace(a, r);
	return CReduce(r, { a }, CTNTYPE_SELF_LOOP);
}

Node CTranslater::ReduceRegionIfElse(NodeSet& N, Node a, Node b, Node c)
{
	Node r = CreateControlTreeNode();
	// r ��ǰ���� a ��ǰ��
	ctrees[r]->pred = ctrees[a]->pred;
	for (auto pred : ctrees[a]->Pred())
		ctrees[pred]->succ.Replace(a, r);

	// r �ĺ���� b �� c �ĺ�̣��� b �� c ��ֻ��һ����ͬ�ĺ��
	ctrees[r]->succ = ctrees[b]->succ;
	for (auto s : ctrees[b]->Succ())
	{
		ctrees[s]->pred.Replace({ b, c }, { r });
	}

	// ʹ�� r ���� a, b, c
	N.Replace({ a, b, c }, { r });
	return CReduce(r, { a, b, c }, CTNTYPE_IF_ELSE);
}

Node CTranslater::ReduceRegionIfOr(NodeSet& N, Node a, Node b, Node c)
{
	Node r = CreateControlTreeNode();
	// r ��ǰ���� a ��ǰ��
	ctrees[r]->pred = ctrees[a]->pred;
	for (auto pred : ctrees[a]->Pred())
		ctrees[pred]->succ.Replace(a, r);

	// r �ĺ���� b �� c �ĺ�� d���� b -> c, b -> d, c -> d
	ctrees[r]->succ = ctrees[c]->succ;
	for (auto s : ctrees[r]->Succ())
	{
		ctrees[s]->pred.Replace({ b, c }, { r });
	}

	// ʹ�� r ���� a, b, c
	N.Replace({ a, b, c }, { r });
	return CReduce(r, { a, b, c }, CTNTYPE_IF_OR);
}

Node CTranslater::ReduceRegionIf(NodeSet& N, Node a, Node b)
{
	Node r = CreateControlTreeNode();
	// r ��ǰ���� a ��ǰ��
	ctrees[r]->pred = ctrees[a]->pred;
	for (auto pred : ctrees[a]->Pred())
		ctrees[pred]->succ.Replace(a, r);

	// r �ĺ���� a �� b �ĺ�̣��� a �� b ��ֻ��һ����ͬ�ĺ��
	ctrees[r]->succ = ctrees[b]->succ;
	for (auto s : ctrees[b]->Succ())
	{
		ctrees[s]->pred.Replace({ a, b }, { r });
	}

	// ʹ�� r ���� a, b
	N.Replace({ a, b }, { r });
	return CReduce(r, { a, b }, CTNTYPE_IF);
}

Node CTranslater::ReduceRegionPoint2Loop(NodeSet& N, Node a, Node b)
{
	//COUT << "2��ѭ��:\n";
	//ctrees[a]->pred.Dump();
	//COUT << endl;
	//ctrees[b]->succ.Dump();
	//COUT << endl;
	Node r = CreateControlTreeNode();
	// r ��ǰ���� a ���� b ֮���ǰ��
	ctrees[r]->pred = ctrees[a]->pred - b;

	for (auto pred : ctrees[r]->Pred())
		ctrees[pred]->succ.Replace(a, r);

	// r �ĺ���� b ���� a ֮��ĺ��
	ctrees[r]->succ = ctrees[b]->succ - a;
	for (auto s : ctrees[r]->Succ())
		ctrees[s]->pred.Replace(b, r);

	// a �ĳ� b ֮��ĺ�̷���Ϊ goto ���
	for (auto s : ctrees[a]->Succ())
	{
		if (s == b)
			continue;
		// a goto s �����ߵ�gotoӦ����Ҷ�ӽڵ㵽Ҷ�ӽڵ�ı�
		// COUT << a << " goto " << s << endl;
		// �Ƴ�������
		ctrees[a]->succ -= s;
		ctrees[s]->pred -= a;
		// GetLeafEdges({ a, s });
		/*SetSub(ctrees[s]->pred, a);
		SetUnion(ctrees[s]->pred, r);*/
	}

	// �滻 a��b Ϊ r
	N.Replace({ a, b }, { r });
	return CReduce(r, { a, b }, CTNTYPE_P2LOOP);
}

void CTranslater::GetLeafEntry(ControlTreeNodeEx* node, vector<Node>& nodes)
{
	switch (node->type)
	{
	case CTNTYPE_LEAF:
		nodes.push_back(node->index);
		break;
	case CTNTYPE_LIST:
	case CTNTYPE_P2LOOP:
		GetLeafEntry(node->pair.first, nodes);
		break;
	case CTNTYPE_SELF_LOOP:
		GetLeafEntry(node->node, nodes);
		break;
	case CTNTYPE_IF:
		GetLeafEntry(node->_if.condition, nodes);
		break;
	case CTNTYPE_IF_ELSE:
		GetLeafEntry(node->_if.condition, nodes);
		break;
	default:
		throw Exception(_T("��ȡҶ�����ʧ��"));
	}
}

void CTranslater::GetLeafExit(ControlTreeNodeEx* node, vector<Node>& nodes)
{
	switch (node->type)
	{
	case CTNTYPE_LEAF:
		nodes.push_back(node->index);
		break;
	case CTNTYPE_LIST:
	case CTNTYPE_P2LOOP:
		GetLeafEntry(node->pair.second, nodes);
		break;
	case CTNTYPE_SELF_LOOP:
		GetLeafEntry(node->node, nodes);
		break;
	case CTNTYPE_IF:
		GetLeafEntry(node->_if.condition, nodes);
		GetLeafEntry(node->_if.then, nodes);
		break;
	case CTNTYPE_IF_ELSE:
		GetLeafEntry(node->_if.then, nodes);
		GetLeafEntry(node->_if._else, nodes);
		break;
	default:
		throw Exception(_T("��ȡҶ�ӳ���ʧ��"));
	}
}

std::vector<Edge> CTranslater::GetLeafEdges(Edge e)
{
	vector<Edge> result(32);
	result.clear();
	vector<Node> entries;
	vector<Node> exits;
	GetLeafEntry(ctrees[e.from], entries);
	GetLeafExit(ctrees[e.to], exits);
	for (auto from : entries)
	{
		for (auto to : exits)
		{
			result.push_back({ from, to });
		}
	}
	return result;
}

// ������ڵ�ת��Ϊ�������ڵ�
void CTranslater::InitializeBaseControlTree(NodeSet N)
{
	//memset(ctrees, 0, sizeof(ctrees));
	for (auto n : Nodes(N))
	{
		ctrees[n] = tempAllocator.New<ControlTreeNodeEx>();
		*(BasicBlock*)ctrees[n] = blocks[n];
		ctrees[n]->type = CTNTYPE_LEAF;
	}
	controlTreeNodeCount = blockCount;
}


NodeSet CTranslater::CAnalysis(NodeSet N)
{
	// ���Ƚ������鹹�ɵĿ�����ͼת��Ϊ�������ڵ㹹�ɵĿ�����ͼ
	InitializeBaseControlTree(N);

	NodeSet old = 0;
	while (true)
	{
		if (old == N)
		{
			break;
		}
		old = N;
		for (auto n : Nodes(N))
		{
			switch (ctrees[n]->GetSuccCount())
			{
			case 1:
			{
					  Node succ = ctrees[n]->Succ()[0];
					  if (ctrees[succ]->GetPredCount() == 1)
					  {
						  ReduceRegionList(N, n, succ);
						  // ��һ��ѭ��
						  goto NEXT;
					  }
					  break;
			}
			case 2:
			{
					  auto succ = ctrees[n]->Succ();
					  auto b = ctrees[succ[0]];
					  auto c = ctrees[succ[1]];
					  // a -> b, a -> c, b -> d, c -> d ��ԼΪ if else �ṹ
					  if (b->succ == c->succ && b->GetSuccCount() == 1 &&
						  b->GetPredCount() == 1 && c->GetPredCount() == 1)
					  {
						  ReduceRegionIfElse(N, n, b->index, c->index);
						  goto NEXT;
					  }
					  // a -> b, a -> c, b -> d, c -> d, b -> c ��ԼΪ if (x || y)
					  if ((b->succ & c->succ) != 0)  // b �� c ����ͬ�ĺ��
					  {
						  if (b->GetSuccCount() == 2 && c->GetSuccCount() == 1 && b->succ.Contains(c->index))
						  {
							  // b -> c �ı߷���Ϊ goto
							  ReduceRegionIfOr(N, n, b->index, c->index);
							  goto NEXT;
						  }
						  if (c->GetSuccCount() == 2 && b->GetSuccCount() == 1 && c->succ.Contains(b->index))
						  {
							  ReduceRegionIfOr(N, n, c->index, b->index);
							  // c -> b �ı߷���Ϊ goto
							  goto NEXT;
						  }
					  }

					  if (b->GetPredCount() == 1 && b->GetSuccCount() == 1 &&
						  b->succ.Contains(c->index))
					  {
						  ReduceRegionIf(N, n, b->index);
						  //DumpCurrentCFG(N);
						  goto NEXT;
					  }
					  if (c->GetPredCount() == 1 && c->GetSuccCount() == 1 &&
						  c->succ.Contains(b->index))
					  {
						  ReduceRegionIf(N, n, c->index);
						  //DumpCurrentCFG(N);
						  goto NEXT;
					  }
					  // ��� if

					  break;
			}
			}
			// ѭ�����
			for (auto s : ctrees[n]->Succ())
			{
				if (s == n)  // ��ѭ�����
				{
					//DumpCurrentCFG(N);
					ReduceRegionSelfLoop(N, n);
					//DumpCurrentCFG(N);
					goto NEXT;
				}
				// ����ѭ�� a -> b && b -> a ���� b ֻ��һ��ǰ��
				if (ctrees[s]->succ.Contains(n) && ctrees[s]->GetPredCount() == 1)
				{
					ReduceRegionPoint2Loop(N, n, s);
					goto NEXT;
				}
			}
		}

	NEXT:
		;
	}
	return N;
}

void CTranslater::DumpCFG()
{
	for (int i = 0; i < blockCount; ++i)
	{
		auto block = &blocks[i];
		COUT << _T("block ") << i << _T(" , ǰ�� : ");
		DumpNodeSet(block->pred);
		COUT << _T(" ��� : ");
		DumpNodeSet(block->succ);
		COUT << endl;
	}
}

void CTranslater::DumpControlTree()
{
	for (int i = 0; i < controlTreeNodeCount; ++i)
	{
		auto block = ctrees[i];
		COUT << _T("tree node ") << i << _T(" , ǰ�� : ");
		DumpNodeSet(block->pred);
		COUT << _T(" ��� : ");
		DumpNodeSet(block->succ);
		COUT << endl;
	}
}

void CTranslater::DumpCurrentCFG(NodeSet& N)
{
	auto nodes = Nodes(N);
	for (auto i : nodes)
	{
		auto node = ctrees[i];
		COUT << _T("tree node ") << i << _T(" , ǰ�� : ");
		DumpNodeSet(node->pred);
		COUT << _T(" ��� : ");
		DumpNodeSet(node->succ);
		COUT << endl;
	}
}
