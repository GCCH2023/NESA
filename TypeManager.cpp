#include "stdafx.h"
#include "TypeManager.h"
using namespace std;

static Type Void(TypeKind::Void);
static Type Char(TypeKind::Char);
static Type Short(TypeKind::Short);
static Type Int(TypeKind::Int);
static Type Long(TypeKind::Long);
static Type LongLong(TypeKind::LongLong);
static Type UnsignedChar(TypeKind::UChar);
static Type UnsignedShort(TypeKind::UShort);
static Type UnsignedInt(TypeKind::UInt);
static Type UnsignedLong(TypeKind::ULong);
static Type UnsignedLongLong(TypeKind::ULongLong);
static Type Float(TypeKind::Float);
static Type Double(TypeKind::Double);
static Type LongDouble(TypeKind::LongDouble);

Type* TypeManager::Void = &::Void;
Type* TypeManager::Char = &::Char;
Type* TypeManager::Short = &::Short;
Type* TypeManager::Int = &::Int;
Type* TypeManager::Long = &::Long;
Type* TypeManager::LongLong = &::LongLong;
Type* TypeManager::UnsignedChar = &::UnsignedChar;
Type* TypeManager::UnsignedShort = &::UnsignedShort;
Type* TypeManager::UnsignedInt = &::UnsignedInt;
Type* TypeManager::UnsignedLong = &::UnsignedLong;
Type* TypeManager::UnsignedLongLong = &::UnsignedLongLong;
Type* TypeManager::Float = &::Float;
Type* TypeManager::Double = &::Double;
Type* TypeManager::LongDouble = &::LongDouble;


TypeManager::TypeManager(Allocator& allocator_) :
allocator(allocator_)
{

}

Type* TypeManager::NewArray(Type* elementType, size_t count)
{
	Type type(TypeKind::Array);
	type.pa.type = elementType;
	type.pa.count = count;
	return GetType(&type);
}

Type* TypeManager::NewPointer(Type* pointerToType)
{
	Type type(TypeKind::Pointer);
	type.pa.type = pointerToType;
	return GetType(&type);
}

Type* TypeManager::NewStruct(String* name /*= nullptr*/, Field* fileds /*= nullptr*/)
{
	Type* type = allocator.New<Type>(TypeKind::Struct);
	type->su.name = name;
	type->su.fields = fileds;
	return type;
}

Type* TypeManager::NewUnion(String* name /*= nullptr*/, Field* fileds /*= nullptr*/)
{
	Type* type = allocator.New<Type>(TypeKind::Union);
	type->su.name = name;
	type->su.fields = fileds;
	return type;
}

Type* TypeManager::NewEnum(String* name /*= nullptr*/, Enumerator* members /*= nullptr*/)
{
	Type* type = allocator.New<Type>(TypeKind::Enum);
	type->e.name = name;
	type->e.members = members;
	return type;
}

Type* TypeManager::NewFunction(Type* functionType)
{
	assert(functionType);
	assert(functionType->GetKind() == TypeKind::Function);
	return GetType(functionType);
}

Type* TypeManager::GetType(Type* type)
{
	auto it = types.find(type);
	if (it != types.end())
		return *it;
	auto t = allocator.New<Type>(type->GetKind());
	switch (t->GetKind())
	{
	case TypeKind::Function:
	{
							   t->f.returnType = type->f.returnType;
							   // 需要分配参数对象
							   for (auto p = type->f.params; p; p = p->next)
							   {
								   auto pa = allocator.New<TypeList>();
								   pa->type = p->type;
								   t->AddParameter(pa);
							   }
							   break;
	}

	}
	types.insert(t);
	return t;
}

std::size_t TypeHash::operator()(const Type* type) const
{
	size_t h = hash<uint32_t>()(type->base);
	// 因为其他字段共享一段内存空间，所以直接计算就行了
	h = h ^ hash<size_t>()((size_t)(type->pa.type)) ^ hash<size_t>()(type->pa.count);
	return h;
}

bool TypeEqual::operator()(const Type* type1, const Type* type2) const
{
	if (type1->base != type2->base)
		return false;
	switch (type1->GetKind())
	{
	case TypeKind::Array: return memcmp(&type1->pa, &type2->pa, sizeof(type1->pa)) == 0;
	case TypeKind::Pointer: return type1->pa.type == type2->pa.type;
	case TypeKind::Union: return type1->su.name == type2->su.name;
	case TypeKind::Struct: return type1->su.name == type2->su.name;
	case TypeKind::Function:
	{
							   if (type1->f.returnType != type2->f.returnType)
								   return false;
							   auto p = type1->f.params;
							   auto q = type2->f.params;
							   while (p && q)
							   {
								   if (p->type != q->type)
									   return false;
								   p = p->next;
								   q = q->next;
							   }
							   return !(p || q);  // 只有两个都是nullptr才相等
	}
	default:
		return false;
	}
}
