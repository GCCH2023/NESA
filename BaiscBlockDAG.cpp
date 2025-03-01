#include "stdafx.h"
#include "BaiscBlockDAG.h"
#include "TAC.h"
#include "CNode.h"

BaiscBlockDAG::BaiscBlockDAG(Allocator& allocator_):
allocator(allocator_)
{

}

BaiscBlockDAG::~BaiscBlockDAG()
{
}

CStr BaiscBlockDAG::NewString(const CStr format, ...)
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


CNode* BaiscBlockDAG::GetExpression(TACOperand& operand)
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


CNode* BaiscBlockDAG::Translate(TACBasicBlock* tacBlock)
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
								  // �£������������������ת���������
								  pCondition = allocator.New<CNode>(1);
								  jumpAddr = tac->z.GetValue();
								  continue;

								  // �ɣ� goto �ڿ�����ͼ�ж�Ӧһ���ߣ����ܱ������ѭ���ṹ��Ҳ���ܾ��Ƕ�Ӧgoto��䣬
								  // ����֪������ô����
								  //if (tac->z.GetValue() == tac->address)
								  //{
								  // // ��ת���Լ�����䷭��Ϊ while (1);
								  // //expr = allocator.New<CInteger>(1);
								  // //current = allocator.New<CWhileStatement>(expr, noneStatement);
								  // current = noneStatement;
								  // pCondition = allocator.New<CNode>(1);
								  // break;
								  //}
								  //auto label = GetLabelName(tac->z.GetValue());
								  //current = allocator.New<CNode>(CNodeKind::STAT_GOTO, label);
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
