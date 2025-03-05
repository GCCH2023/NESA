#include "stdafx.h"
#include "CDataBase.h"

CDataBase::CDataBase(Allocator& allocator_) :
allocator(allocator_),
stringTable(allocator_),
typeManager(allocator_)
{
	// ���PPU�Ĵ���ӳ���ڴ��ַ��Ϊȫ�ֱ���
	for (CAddress i = Nes::PPU_CTRL; i <= Nes::PPU_DATA; ++i)
	{
		auto name = AddString(ToString((Nes::PPURegister)i));
		RawAddGlobalVariable(name, TypeManager::UnsignedChar, i);
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

const Variable* CDataBase::AddGlobalVariable(String* name, Type* type, CAddress address)
{
	// Ӧ�ñ�֤��ַ������Ψһ
	const Variable* variable = GetGlobalVariable(name);
	if (variable)
	{
		Sprintf<> s;
		s.Format(_T("����Ϊ %s ��ȫ�ֱ����Ѿ�����"), name);
		throw Exception(s.ToString());
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

void CDataBase::AddFunction(Function* function)
{
	if (!function)
		throw Exception(_T("Ҫ��ӵĺ���Ϊ nullptr"));

	functions.push_back(function);
}

void CDataBase::RawAddGlobalVariable(String* name, Type* type, CAddress address)
{
	auto it = std::lower_bound(globals.begin(), globals.end(), address,
		[](const Variable* variable, CAddress address) {
		return variable->address < address;
	});
	Variable* v = allocator.New<Variable>();
	v->address = address;
	v->name = name;
	v->type = type;
	globals.insert(it, v);
}
