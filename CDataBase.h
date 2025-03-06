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
	friend CDataBase& GetCDB();
private:
	CDataBase(Allocator& allocator);
public:
	~CDataBase();

	// ���ָ���ַ������ַ�������
	inline String* AddString(const TCHAR* str) { return stringTable.AddString(str); }
	// ��ȡָ���ַ������󣬲������򷵻ؿ�
	inline String* GetString(const TCHAR* str) { return stringTable.GetString(str); }

	// ��ȡȫ�ֱ����б�
	inline const GlobalList& GetGlobalList() const { return globals; }
	// ���ݵ�ַ��ȡȫ�ֱ����������ڷ��� nullptr
	const Variable* GetGlobalVariable(CAddress address) const;
	// �������ƻ�ȡȫ�ֱ�������������ڣ����� nullptr
	const Variable* GetGlobalVariable(String* name) const;
	// ���һ��ȫ�ֱ�����ʧ���׳��쳣
	// ������������NES����ģ����Ե�ַ�����ͱ���Ҫ��
	// ���û�����ƣ����Զ���������
	const Variable* AddGlobalVariable(CAddress address, Type* type, String* name = nullptr);
	// ���ݵ�ַ����ȫ�ֱ���������
	String* MakeGlobalVariableName(CAddress address);

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

	// ��ȡ������
	inline Allocator& GetAllocator() { return allocator; }
protected:
	// �����κ���֤�����ȫ�ֱ���
	void RawAddGlobalVariable(CAddress address, Type* type, String* name = nullptr, void* initializer = nullptr);
protected:
	Allocator& allocator;
	StringTable stringTable;

	GlobalList globals;  // ȫ�ֱ����б�����ַ��С��������
	FunctionList functions;  // �����б�
	TagList tags;  // �ṹ�壬ö�٣��������б�
};

// ��ȡC���ݿⵥ��
CDataBase& GetCDB();