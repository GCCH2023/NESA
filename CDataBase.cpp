#include "stdafx.h"
#include "CDataBase.h"

CDataBase::CDataBase(Allocator& allocator_) :
allocator(allocator_),
stringTable(allocator_),
axyType(nullptr)
{
	// ���PPU�Ĵ���ӳ���ڴ��ַ��Ϊȫ�ֱ���
	for (CAddress i = Nes::PPU_CTRL; i <= Nes::PPU_DATA; ++i)
	{
		auto name = AddString(ToString((Nes::PPURegister)i));
		RawAddGlobalVariable(i, TypeManager::UnsignedChar, name);
	}
}

CDataBase::~CDataBase()
{
}

const Variable* CDataBase::GetGlobalVariable(CAddress address) const
{
	auto it = std::lower_bound(globals.begin(), globals.end(), address,
		[](const Variable* variable, CAddress address) {
		return variable->address < address;
	});
	// �ҵ��ͷ��ظö���
	if (it != globals.end() && (*it)->address == address)
		return *it;
	return nullptr;
}

const Variable* CDataBase::GetGlobalVariable(String* name) const
{
	for (auto global : globals)
	{
		if (global->name == name)
			return global;
	}
	return nullptr;
}

const Variable* CDataBase::AddGlobalVariable(CAddress address, Type* type, String* name /*= nullptr*/)
{
	// Ӧ�ñ�֤��ַ������Ψһ
	if (name)
	{
		const Variable* variable = GetGlobalVariable(name);
		if (variable)
		{
			Sprintf<> s;
			s.Format(_T("����Ϊ %s ��ȫ�ֱ����Ѿ�����"), name);
			throw Exception(s.ToString());
		}
	}
	else
	{
		name = MakeGlobalVariableName(address);
	}

	auto it = std::lower_bound(globals.begin(), globals.end(), address,
		[](const Variable* variable, CAddress address) {
		return variable->address < address;
	});
	if (it != globals.end() && (*it)->address == address)
	{
		Sprintf<> s;
		s.Format(_T("��ַΪ %X ��ȫ�ֱ����Ѿ�����"), address);
		throw Exception(s.ToString());
	}

	// ����һ��ȫ�ֱ���
	// �Ҳ����ʹ�������ӵ��б���
	Variable* v = allocator.New<Variable>();
	v->address = address;
	v->name = name;
	v->type = type;
	globals.insert(it, v);
	return v;
}

String* CDataBase::MakeGlobalVariableName(CAddress address)
{
	// �Զ���������
	Sprintf<> s;
	s.Format(_T("g_%04X"), address);
	return AddString(s.ToString());
}

void CDataBase::AddFunction(Function* function)
{
	if (!function)
		return;

	functions.push_back(function);
}

void CDataBase::AddTag(Type* tag)
{
	if (!tag)
		return;
	tags.push_back(tag);
}

Type* CDataBase::GetTag(String* name)
{
	for (auto t : tags)
	{
		switch (t->GetKind())
		{
		case TypeKind::Struct:
		case TypeKind::Union:
			if (t->su.name == name)
				return t;
			break;
		case TypeKind::Enum:
			if (t->e.name == name)
				return t;
			break;
		}
	}
	return nullptr;
}

void CDataBase::RawAddGlobalVariable(CAddress address, Type* type, String* name /*= nullptr*/, void* initializer /*= nullptr*/)
{
	auto it = std::lower_bound(globals.begin(), globals.end(), address,
		[](const Variable* variable, CAddress address) {
		return variable->address < address;
	});
	if (!name)
		name = MakeGlobalVariableName(address);
	Variable* v = allocator.New<Variable>();
	v->address = address;
	v->name = name;
	v->type = type;
	v->initializer = initializer;
	globals.insert(it, v);
}




Type* CDataBase::GetAXYType()
{
	if (axyType)
		return axyType;

	Field* fieldA = allocator.New<Field>();
	fieldA->name = AddString(_T("A"));
	fieldA->align = GetTypeAlign(TypeManager::Char);
	fieldA->type = TypeManager::Char;

	Field* fieldX = allocator.New<Field>();
	fieldX->name = AddString(_T("X"));
	fieldX->align = GetTypeAlign(TypeManager::Char);
	fieldX->type = TypeManager::Char;

	Field* fieldY = allocator.New<Field>();
	fieldY->name = AddString(_T("Y"));
	fieldY->align = GetTypeAlign(TypeManager::Char);
	fieldY->type = TypeManager::Char;

	fieldA->next = fieldX;
	fieldX->next = fieldY;

	axyType = GetTypeManager().NewStruct(GetCDB().AddString(_T("AXY")), fieldA);
	AddTag(axyType);
	return axyType;
}


CDataBase& GetCDB()
{
	static CDataBase cdb(GetTypeManager().GetAllocator());
	return cdb;
}
