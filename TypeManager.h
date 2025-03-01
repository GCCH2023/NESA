#pragma once
#include "Type.h"

class Allocator;
struct String;

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
	static Type* Void;
	static Type* Char;
	static Type* Short;
	static Type* Int;
	static Type* Long;
	static Type* LongLong;
	static Type* UnsignedChar;
	static Type* UnsignedShort;
	static Type* UnsignedInt;
	static Type* UnsignedLong;
	static Type* UnsignedLongLong;
	static Type* Float;
	static Type* Double;
	static Type* LongDouble;

	TypeManager(Allocator& allocator);
	// ����һ����������
	Type* NewArray(Type* elementType, size_t count);
	// ����һ��ָ������
	Type* NewPointer(Type* pointerToType);
	// ����һ���ṹ��
	Type* NewStruct(String* name = nullptr, Field* fileds = nullptr);
	// ����һ��������
	Type* NewUnion(String* name = nullptr, Field* fileds = nullptr);
	// ����һ��ö��
	Type* NewEnum(String* name = nullptr, Enumerator* members = nullptr);
	// ����һ�����������������ǿ��Ը��õ�
	Type* NewFunction(Type* functionType);

protected:
	// �������ָ�����ͣ��򷵻����еģ����򴴽��µķ���
	Type* GetType(Type* type);
protected:
	Allocator& allocator;
	std::unordered_set<Type*, TypeHash, TypeEqual> types;
};
