#include "stdafx.h"
#include "CDataBase.h"

CDataBase::CDataBase(Allocator& allocator_) :
allocator(allocator_),
stringTable(allocator_),
typeManager(allocator_)
{
	// 添加PPU寄存器映射内存地址作为全局变量
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
	// 找到就返回该对象
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
	// 应该保证地址和名称唯一
	const Variable* variable = GetGlobalVariable(name);
	if (variable)
	{
		Sprintf<> s;
		s.Format(_T("名称为 %s 的全局变量已经存在"), name);
		throw Exception(s.ToString());
	}

	auto it = std::lower_bound(globals.begin(), globals.end(), address,
		[](const Variable* variable, CAddress address) {
		return variable->address < address;
	});
	if (it != globals.end() && (*it)->address == address)
	{
		Sprintf<> s;
		s.Format(_T("地址为 %X 的全局变量已经存在"), address);
		throw Exception(s.ToString());
	}

	// 创建一个全局变量
	// 找不到就创建并添加到列表中
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
		throw Exception(_T("要添加的函数为 nullptr"));

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
