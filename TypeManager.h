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
	// C ���Ի�������
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

	// FC ר������
	static Type* Value;  // ֵ�����ͣ�һ���ֽ�
	static Type* pValue;  // ָ��ֵ��ָ�룬ռ2���ֽڣ����ڱ�ַѰַ
	static Type* ppValue;  // ָ��ֵ��2��ָ�룬ռ2���ֽڣ����ڼ�ӱ�ַѰַ

private:
	friend TypeManager& GetTypeManager();
	TypeManager(Allocator& allocator);

public:
	// ��ȡ���͹�����ʹ�õķ�����
	inline Allocator& GetAllocator() { return allocator; }

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

// ��ȡ���͹�������ȫ��ʵ��
TypeManager& GetTypeManager();