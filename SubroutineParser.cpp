#include "stdafx.h"
#include "SubroutineParser.h"
#include "NesDataBase.h"
using namespace NesDB;
using namespace Nes;

NesDB::SubroutineParser::SubroutineParser(NesDataBase& db_):
    db(db_)
{
}

NesSubroutine* SubroutineParser::Parse(Nes::Address start)
{
	auto subroutine = db.FindSubroutine(start);
	if (subroutine)
		return subroutine;

    Reset();

	Instruction instruction;
	// 从卡带中获取函数开头指针
	const uint8_t* p = db.GetCartridge().GetData(start);
	Address current = start;  // 当前分析的指令地址
	Address end = 0x10000;
	Address jumpAddr;
	int bytes;

	std::vector<Address> worklist;
	subroutine = db.GetAllocator().New <NesSubroutine>();
	subroutine->SetStartAddress(start);

	worklist.push_back(start);
	while (!worklist.empty())
	{
		Address currentAddr = worklist.back();
		worklist.pop_back();

		if (db.GetBasicBlock(currentAddr))  // 数据库中包含该基本块了
			continue;

		NesBasicBlock* block = db.GetAllocator().New<NesBasicBlock>();
		block->SetStartAddress(currentAddr);

		current = currentAddr;
		bool blockEnded = false;

		while (!blockEnded && current < end)
		{
			instruction.Set(current, p);  // 构造指令对象
			const auto& entry = instruction.GetEntry();
			bytes = entry.length;
			current += bytes;  // 计算下一条指令的地址

			switch (entry.opcode)
			{
			case Opcode::Jmp:
				blockEnded = true;
				if (entry.addrMode == AddrMode::Absolute)
				{
					jumpAddr = instruction.GetOperandAddress();
					worklist.push_back(jumpAddr);
					block->AddSucc(jumpAddr);
				}
				throw Exception(_T("未实现"));
				break;
			case Opcode::Bpl:
			case Opcode::Bmi:
			case Opcode::Bne:
			case Opcode::Beq:
			case Opcode::Bcc:
			case Opcode::Bcs:
			case Opcode::Bvc:
			case Opcode::Bvs:
				blockEnded = true;
				jumpAddr = instruction.address + entry.length;
				worklist.push_back(jumpAddr);
				block->AddSucc(jumpAddr);
				// 处理条件为真的情况
				jumpAddr += (char)instruction.GetByte();
				worklist.push_back(jumpAddr);
				block->AddSucc(jumpAddr);
				break;
			case Opcode::Rts:
			case Opcode::Rti:
				blockEnded = true;
				break;
			}
		}

		block->SetEndAddress(current);
		subroutine->AddBasicBlock(block);
	}

	db.AddSubroutine(subroutine);
    return nullptr;
}

void SubroutineParser::Reset()
{

}
