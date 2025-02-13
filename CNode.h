#pragma once

enum class CNodeKind
{
	NONE,  // δȷ��
	NORMAL,  // ��ͨ���

	// ���ʽ����
	EXPR_VARIABLE,  // ����
	EXPR_INTEGER,  // ��������

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

	EXPR_ADD,  // �ӷ�
	EXPR_SUB,  // ����
	EXPR_ASSIGN, // ��ֵ
	EXPR_BOR,  // λ�� |
	EXPR_BAND,  // λ�� &

	EXPR_GREAT,  // ���� >
	EXPR_GREAT_EQUAL,  // ���ڵ��� >=
	EXPR_NOT_EQUAL,  // ������ !=
	EXPR_EQUAL,  // ���� ==
	EXPR_LESS,  // С�� <
	EXPR_LESS_EQUAL,  // С�ڵ��� <=

	EXPR_INDEX,  // ���� z = x[y]
	EXPR_REF,  // ������ *p

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

	CNode(CNodeKind kind, uint32_t address);
};

// C���Ա��ʽ�ڵ�
struct CExpression : public CNode
{
	CExpression(CNodeKind kind, uint32_t address = 0);
};

// ��Ŀ���ʽ
struct CSingleExpression : public CExpression
{
	CExpression* operand;

	CSingleExpression(CNodeKind kind, CExpression* operand);
};


// ˫Ŀ���ʽ
struct CBinaryExpression : public CExpression
{
	CExpression* left;
	CExpression* right;

	CBinaryExpression(CNodeKind kind, CExpression* left, CExpression* right);
};

enum VariableKind
{
	VAR_KIND_GLOBAL,  // ȫ�ֱ���
	VAR_KIND_LOCAL,  // �ֲ�����
	VAR_KIND_FUNCTION,  // ����
};

// ����
struct CVariable : public CExpression
{
	String name;
	VariableKind varKind;

	CVariable(LPCTSTR name, VariableKind kind = VAR_KIND_GLOBAL);
};

// ��������
struct CInteger : public CExpression
{
	int value;

	CInteger(int value);
};

// C�������ڵ�
struct CStatement : public CNode
{
	CStatement(CNodeKind kind, uint32_t address = 0);
};

// �������
struct CListStatement : public CStatement
{
	CStatement* first;
	CStatement* second;

	CListStatement(CStatement* first = nullptr, CStatement* second = nullptr);
};

// ���ʽ���
struct CExpressionStatement : public CStatement
{
	CExpression* expression;

	CExpressionStatement(CExpression* expression = nullptr);
};

// �����������
struct CCallStatement : public CStatement
{
	CVariable* funcName;  // ��������
	CExpression** args;  // �����������飬�� �ս�β

	CCallStatement(CVariable* funcName = nullptr);
};

// �����������
struct CWhileStatement : public CStatement
{
	CExpression* expression;
	CStatement* body;

	CWhileStatement(CExpression* expression = nullptr, CStatement* body = nullptr);
};

// �����������
struct CDoWhileStatement : public CStatement
{
	CExpression* expression;
	CStatement* body;

	CDoWhileStatement(CExpression* expression = nullptr, CStatement* body = nullptr);
};

struct CIfStatement : public CStatement
{
	CExpression* condition;
	CStatement* body;
	CStatement* _else;

	CIfStatement(CExpression* condition = nullptr, CStatement* body = nullptr, CStatement* _else = nullptr);
};

struct CLabelStatement : public CStatement
{
	CStatement* statement;
	String name;

	CLabelStatement(LPCTSTR name, CStatement* statement = nullptr);
};


struct CGotoStatement : public CStatement
{
	String label;

	CGotoStatement(LPCTSTR label = nullptr);
};

struct CReturnStatement : public CStatement
{
	CExpression* value;  // ����ֵ���մ���û�з���ֵ

	CReturnStatement(CExpression* value = nullptr);
};


class Function
{
public:

	inline void SetBody(CStatement* body) { this->body = body; }
	inline CStatement* GetBody() { return body; }

public:
	CStatement* body;  // ������
	String name;
};

// ָ����������ڵ�
OStream& DumpCNode(OStream& os, const CNode* obj, int indent);
// ����ڵ�
OStream& operator<<(OStream& os, const CNode* obj);