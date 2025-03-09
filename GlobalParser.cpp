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

// ����Ѱַ��ʽ����ȷ����ַ��Ӧ��ȫ��
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
		// ���ȿ�һ��ȫ�ֱ����Ƿ���ڣ����ڵĻ���Ҫ�����Ƿ�Ҫ�ϲ���������
		auto global = GetCDB().GetGlobalVariable(address);
		if (global)
		{
			if (global->type == type)
				continue;
			auto oldSize = GetTypeBytes(global->type);
			auto newSize = GetTypeBytes(type);
			if (newSize > oldSize)  // �����ͱ�ԭ���󣬾�ʹ��������
			{
				// 1. �޸ı�������
				GetCDB().SetGlobalVariableType(address, type);
				// 2. ɾ�������ǵı���
				GetCDB().DeleteGlobalVariables(address + oldSize, address + newSize);
			}
			continue;  // С�ڵ���ԭ�������͵Ļ����ͱ���ԭ��������
		}
		// �����������
		GetCDB().AddGlobalVariable(address, type);
	}
}

void GlobalParser::Reset()
{

}
