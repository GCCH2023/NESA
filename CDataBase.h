#pragma once
#include "String.h"
#include "TypeManager.h"
#include "CNode.h"
#include "Function.h"

using GlobalList = std::vector<Variable*>;
using FunctionList = std::vector<Function*>;
using TagList = std::vector<Type*>;

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
	inline const GlobalList& GetGlobalList() const { return globals; }
	// ���ݵ�ַ��ȡȫ�ֱ����������ڷ��� nullptr
	const Variable* GetGlobalVariable(CAddress address) const;
	// �������ƻ�ȡȫ�ֱ�������������ڣ����� nullptr
	const Variable* GetGlobalVariable(String* name) const;
	// ���һ��ȫ�ֱ�����ʧ���׳��쳣
	const Variable* AddGlobalVariable(String* name, Type* type, CAddress address);


	// ��ȡ�����б�
	inline const FunctionList& GetFunctionList() const { return functions; }
	// ���һ������
	void AddFunction(Function* function);

	// ��ȡ��ǩ�б�
	inline const TagList& GetTagList() const { return tags; }
	// ���һ����ǩ
	void AddTag(Type* tag);
	// �������ƻ�ȡ��ǩ����
	Type* GetTag(String* name);
protected:
	// �����κ���֤�����ȫ�ֱ���
	void RawAddGlobalVariable(String* name, Type* type, CAddress address, void* initializer = nullptr);
protected:
	Allocator& allocator;
	StringTable stringTable;
	TypeManager typeManager;

	GlobalList globals;  // ȫ�ֱ����б�����ַ��С��������
	FunctionList functions;  // �����б�
	TagList tags;  // �ṹ�壬ö�٣��������б�
};

