#pragma once
#include "Variable.h"

struct String;
struct Field;

// !!!���ӽڵ�����ʱ��ע���޸�CNode�е��ж����ͺ���
enum class CNodeKind
{
	NONE,  // δȷ��

	// �������
	STAT_LIST,  // ����б�
	STAT_EXPR,  // ���ʽ���
	STAT_WHILE,  // while ���
	STAT_NONE,  // �����
	STAT_DO_WHILE,  // do while ���
	STAT_IF, // if ���
	STAT_GOTO,
	STAT_LABEL,
	STAT_RETURN,

	// ���ʽ����
	EXPR_VARIABLE,  // ����
	EXPR_FIELD,  // �ֶ�
	EXPR_INTEGER,  // ��������

	EXPR_ADD,  // �ӷ�
	EXPR_SUB,  // ����
	EXPR_ASSIGN, // ��ֵ
	EXPR_BOR,  // λ�� |
	EXPR_BAND,  // λ�� &
	EXPR_XOR,  // ��� ^
	EXPR_SHIFT_LEFT,  // ���� <<
	EXPR_SHIFT_RIGHT,  // ���� >>

	EXPR_GREAT,  // ���� >
	EXPR_GREAT_EQUAL,  // ���ڵ��� >=
	EXPR_NOT_EQUAL,  // ������ !=
	EXPR_EQUAL,  // ���� ==
	EXPR_LESS,  // С�� <
	EXPR_LESS_EQUAL,  // С�ڵ��� <=

	EXPR_INDEX,  // ���� x[y]
	EXPR_ARROW,  // ȡ��¼����ָ����ֶ� x->y
	EXPR_DOT,  // ȡ��¼������ֶ� x.y
	EXPR_DEREF,  // ������ *p
	EXPR_ADDR,  // ȡ��ַ &a
	EXPR_CAST,  // ����ת�� (T)a
	EXPR_CALL,  // ��������

	EXPR_BOR_ASSIGN,  // |= 
	EXPR_BAND_ASSIGN,  // &=

	EXPR_AND,  // &&
	EXPR_OR,  // ||
	EXPR_NOT,  // !
};


// C�����﷨�ڵ�
struct CNode
{
	CNodeKind kind;  // �ڵ������
	uint32_t address;  // ��Ӧ��NES��ַ

	union
	{
		struct
		{
			String* name;
			CNode* body;
		}l;  // ��ǩ���
		const Variable* variable;  // ����
		const Field* field;  // ��¼���͵��ֶ�
		struct
		{
			const Type* type;  // ����ת�����ʽ��Ŀ������
			CNode* expr;
		}cast;  // ����ת�����ʽ
		struct
		{
			int value;
		}i;  // ��������
		struct
		{
			CNode* x;
			CNode* y;
			CNode* z;
		}e;  // ���ʽ������������
		struct
		{
			CNode* condition;
			CNode* then;
			CNode* _else;
		}s;  // if, while, do while
		struct
		{
			String* name;  // ��������
			CNode* params;  // ��������
		}f;
		struct
		{
			CNode* head;
			CNode* tail;
		}list; // ����б�
	};
	CNode* next = nullptr;

	CNode();
	CNode(CNodeKind kind, uint32_t address);
	// ��������
	CNode(const Variable* variable);
	// �����ֶ�
	CNode(const Field* field);
	// ��������ת�����ʽ
	CNode(const Type* type, CNode* expr);
	// �����������û��ǩ���
	CNode(String* name, CNode* params);
	// ��������
	CNode(int value);
	// �������ʽ�����
	CNode(CNodeKind kind, CNode* x = nullptr, CNode* y = nullptr, CNode* z = nullptr);
	// ����goto���
	CNode(CNodeKind kind, String* name);

	// �Ƿ������ڵ�
	bool IsStatement() const { return kind >= CNodeKind::STAT_LIST && kind <= CNodeKind::STAT_RETURN; }
	// �Ƿ��Ǳ��ʽ�ڵ�
	bool IsExpression() const { return kind >= CNodeKind::EXPR_VARIABLE && kind <= CNodeKind::EXPR_NOT; }
};
