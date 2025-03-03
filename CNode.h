#pragma once

struct String;

// !!!���ӽڵ�����ʱ��ע���޸�CNode�е��ж����ͺ���
enum class CNodeKind
{
	NONE,  // δȷ��

	// �������
	STAT_LIST,  // ����б�
	STAT_EXPR,  // ���ʽ���
	STAT_CALL,  // ��������
	STAT_WHILE,  // while ���
	STAT_NONE,  // �����
	STAT_DO_WHILE,  // do while ���
	STAT_IF, // if ���
	STAT_GOTO,
	STAT_LABEL,
	STAT_RETURN,

	// ���ʽ����
	EXPR_VARIABLE,  // ����
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
	EXPR_REF,  // ������ *p
	EXPR_ADDR,  // ȡ��ַ &a

	EXPR_BOR_ASSIGN,  // |= 
	EXPR_BAND_ASSIGN,  // &=

	EXPR_AND,  // &&
	EXPR_OR,  // ||
	EXPR_NOT,  // !
};

enum VariableKind
{
	VAR_KIND_GLOBAL,  // ȫ�ֱ���
	VAR_KIND_LOCAL,  // �ֲ�����
	VAR_KIND_FUNCTION,  // ����
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
		struct
		{
			String* name;  // ������
			VariableKind varKind;
		}v;
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
	CNode(String* name, VariableKind varKind = VAR_KIND_GLOBAL);
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
