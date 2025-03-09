#include "stdafx.h"
#include "GlobalParser.h"
#include "NesDataBase.h"
using namespace Nes;
#include "CDataBase.h"

GlobalParser::GlobalParser(NesDataBase& db_):
db(db_)
{

}

GlobalParser::~GlobalParser()
{
}

// 根据寻址方式可以确定地址对应的全局
void GlobalParser::Parse(NesSubroutine* subroutine)
{
	if (!subroutine)
		return;
	Reset();

	std::vector<Instruction> instructions(256);
	instructions.clear();
	db.GetInstructions(instructions, subroutine->GetStartAddress(), subroutine->GetEndAddress());

	uint32_t address = 0;
	Type* type = nullptr;
	for (auto& i : instructions)
	{
		const OpcodeEntry& entry = GetOpcodeEntry(i.GetOperatorByte());
		switch (entry.addrMode)
		{
		case Nes::ZeroPage:
			address = i.GetByte();
			type = TypeManager::Value;
			break;
		case Nes::ZeroPageX:
			address = i.GetByte();
			type = TypeManager::ValueArray;
			break;
		case Nes::ZeroPageY:
			address = i.GetByte();
			type = TypeManager::ValueArray;
			break;
		case Nes::Absolute:
			address = i.GetOperandAddress();
			type = TypeManager::Value;
			break;
		case Nes::AbsoluteX:
			address = i.GetOperandAddress();
			type = TypeManager::ValueArray;
			break;
		case Nes::AbsoluteY:
			address = i.GetOperandAddress();
			type = TypeManager::ValueArray;
			break;
		case Nes::Indirect:
			address = i.GetOperandAddress();
			type = TypeManager::pValue;
			break;
		case Nes::IndirectX:
			address = i.GetByte();
			type = TypeManager::ValueArray;
			break;
		case Nes::IndirectY:
			address = i.GetByte();
			type = TypeManager::pValue;
			break;
		default:
			continue;
		}
		// 首先看一下全局变量是否存在，存在的话就要考虑是否要合并的问题了
		auto global = GetCDB().GetGlobalVariable(address);
		if (global)
		{
			if (global->type == type)
				continue;
			auto oldSize = GetTypeBytes(global->type);
			auto newSize = GetTypeBytes(type);
			if (newSize > oldSize)  // 新类型比原来大，就使用新类型
			{
				// 1. 修改变量类型
				GetCDB().SetGlobalVariableType(address, type);
				// 2. 删除被涵盖的变量
				GetCDB().DeleteGlobalVariables(address + oldSize, address + newSize);
			}
			continue;  // 小于等于原来的类型的话，就保持原来的类型
		}
		// 不存在则添加
		GetCDB().AddGlobalVariable(address, type);
	}
}

void GlobalParser::Reset()
{

}
