#pragma once
#include "String.h"
#include "TypeManager.h"
#include "CNode.h"
#include "Function.h"

// ����C�����������
class CDataBase
{
public:
	CDataBase(Allocator& allocator);
	~CDataBase();

	// ���ָ���ַ������ַ�������
	inline String* AddString(const TCHAR* str) { return stringTable.AddString(str); }
	// ��ȡָ���ַ������󣬲������򷵻ؿ�
	inline String* GetString(const TCHAR* str) { return stringTable.GetString(str); }

	TypeManager& GetTypeManager() { return typeManager; }
protected:
	Allocator& allocator;
	StringTable stringTable;
	TypeManager typeManager;
};

