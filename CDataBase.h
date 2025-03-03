#pragma once
#include "String.h"
#include "TypeManager.h"
#include "CNode.h"
#include "Function.h"

using GlobalList = std::vector<Variable*>;

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

	// ��ȡȫ�ֱ����б�
	inline const GlobalList& GetGlobals() const { return globals; }
	// ���ݵ�ַ��ȡȫ�ֱ����������ڷ��� nullptr
	const Variable* GetGlobalVariable(CAddress address) const;
	// �������ƻ�ȡȫ�ֱ�������������ڣ����� nullptr
	const Variable* GetGlobalVariable(String* name) const;
	// ���һ��ȫ�ֱ�����ʧ���׳��쳣
	const Variable* AddGlobalVariable(String* name, Type* type, CAddress address);

protected:
	// �����κ���֤�����ȫ�ֱ���
	void RawAddGlobalVariable(String* name, Type* type, CAddress address);
protected:
	Allocator& allocator;
	StringTable stringTable;
	TypeManager typeManager;

	GlobalList globals;  // ȫ�ֱ����б�����ַ��С��������
};

