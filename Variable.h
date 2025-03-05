#pragma once
struct String;
struct Type;

using CAddress = uint32_t;

enum class IdentifierKind
{
	None,  // δ֪
	Global,  // ȫ�ֱ���
	Typedef,  // ���ͱ���
	Tag,  // struct, union, enum
};

struct Variable
{
	String* name;  // ����
	Type* type;  // ����
	union
	{
		Variable* next = nullptr;  // �ֲ�����ʹ�ã�ָ����һ������
		CAddress address;  // ȫ�ֱ���ʹ�ã���Ӧ���ڴ��ַ
	};
	// ������ֵ��ʼֵ�����;���������int����ôָ��һ��int��ֵ
	// �������ͣ���ָ�������ֵ
	void* initializer = nullptr;

	Variable();
	Variable(String* name, Type* type);
	Variable(const Variable* other);
};