#pragma once
struct String;
struct Type;

using CAddress = uint32_t;

struct Variable
{
	String* name;  // ����
	Type* type;  // ����
	union
	{
		Variable* next;  // �ֲ�����ʹ�ã�ָ����һ������
		CAddress address;  // ȫ�ֱ���ʹ�ã���Ӧ���ڴ��ַ
	};

	Variable();
	Variable(String* name, Type* type);
	Variable(const Variable* other);
};
