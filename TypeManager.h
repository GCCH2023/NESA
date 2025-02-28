#pragma once
#include "Type.h"

class Allocator;

struct TypeHash
{
	std::size_t operator()(const Type* type) const;
};

// �Զ���ȽϺ���
struct TypeEqual
{
	bool operator()(const Type* type1, const Type* type2) const;
};

// ���͹�����
// �ṩ���������Ͷ��󲢸������Ͷ���
class TypeManager
{
public:
	Type* Void;
	Type* Char;
	Type* Short;
	Type* Int;
	Type* Long;
	Type* LongLong;
	Type* UnsignedChar;
	Type* UnsignedShort;
	Type* UnsignedInt;
	Type* UnsignedLong;
	Type* UnsignedLongLong;
	Type* Float;
	Type* Double;
	Type* LongDouble;

	TypeManager(Allocator& allocator);
	// ����һ����������
	Type* NewArray(Type* elementType, size_t count);
	// ����һ��ָ������
	Type* NewPointer(Type* pointerToType);
	// ����һ���ṹ��
	Type* NewStruct(const TCHAR* name = nullptr, Field* fileds = nullptr);
	// ����һ��������
	Type* NewUnion(const TCHAR* name = nullptr, Field* fileds = nullptr);
	// ����һ��ö��
	Type* NewEnum(const TCHAR* name = nullptr, Enumerator* members = nullptr);
	// ����һ�����������������ǿ��Ը��õ�
	Type* NewFunction(Type* functionType);

protected:
	// �������ָ�����ͣ��򷵻����еģ����򴴽��µķ���
	Type* GetType(Type* type);
protected:
	Allocator& allocator;
	std::unordered_set<Type*, TypeHash, TypeEqual> types;
};
