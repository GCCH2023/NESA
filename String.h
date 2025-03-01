#pragma once

// �ַ���
struct String
{
	String* next;  // ��һ���ַ���
	const TCHAR* str;  // �ַ�������
	uint32_t hash;  // ��ϣֵ
};

// �����ַ�����Ӧ�Ĺ�ϣֵ
uint32_t GetStringHash(const TCHAR* str, size_t* pLen);

#define STR_TABLE_CAPACITY 0x7FF

// �ַ�����
class StringTable
{
public:
	StringTable(Allocator& allocator);

	// ���ָ���ַ������ַ������У�������ڣ��������еģ����򴴽�
	String* AddString(const TCHAR* str);
	// ��ȡָ���ַ������󣬲������򷵻ؿ�
	String* GetString(const TCHAR* str);

protected:
	Allocator& allocator;
	String** table;  // �ַ�����ϣ��
};