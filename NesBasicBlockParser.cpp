#include "stdafx.h"
#include "NesBasicBlockParser.h"
#include "NesDataBase.h"
using namespace Nes;

NesBasicBlockParser::NesBasicBlockParser(NesDataBase& db_, Allocator& allocator_):
db(db_),
allocator(allocator_)
{
}


NesBasicBlockParser::~NesBasicBlockParser()
{
}

std::vector<NesBasicBlock*> NesBasicBlockParser::Parse(Nes::Address start, Nes::Address end)
{
	Reset();

	Instruction instruction;
	// 从卡带中获取函数开头指针
	const uint8_t* p = db.GetCartridge().GetData(start);
	int bytes;


	uint32_t jumpAddr;
	uint32_t current = start;  // 当前分析的指令地址
	// 1. 首先划分基本块
	addrBlockMap.insert({ start, nullptr });
	for (; current < end; p += bytes)
	{
		instruction.Set(current, p);  // 构造指令对象
		const auto& entry = instruction.GetEntry();
		bytes = entry.length;
		current += bytes;  // 计算下一条指令的地址

		switch (entry.opcode)
		{
		case Opcode::Jmp:
			if (entry.addrMode == AddrMode::Absolute)
			{
				jumpAddr = instruction.GetOperandAddress();
				if (jumpAddr >= start && jumpAddr < end)
					addrBlockMap.insert({ jumpAddr, nullptr });
			}
			if (current < end)
				addrBlockMap.insert({ current, nullptr });
			// 间接寻址相当于尾函数调用
			// JMP 同 RTI，RTS 同样处理
			break;
		case Opcode::Bpl:
		case Opcode::Bmi:
		case Opcode::Bne:
		case Opcode::Beq:
		case Opcode::Bcc:
		case Opcode::Bcs:
		case Opcode::Bvc:
		case Opcode::Bvs:
			jumpAddr = instruction.GetConditionalJumpAddress();
			if (jumpAddr >= start && jumpAddr < end)
				addrBlockMap.insert({ jumpAddr, nullptr });
			if (current < end)
				addrBlockMap.insert({ current, nullptr });
			break;
		}
	}

	// 2. 创建基本块
	std::vector<NesBasicBlock*> blocks(addrBlockMap.size());
	blocks.clear();
	addrBlockMap.insert({ end, nullptr });  // 在末尾插入一个元素，以统一操作
	for (auto it = addrBlockMap.begin(); it != addrBlockMap.end();)
	{
		if (it->first == end)
			break;
		NesBasicBlock* block = db.allocator.New<NesBasicBlock>();
		block->SetStartAddress(it->first);
		++it;
		block->SetEndAddress(it->first);
		db.AddBasicBlock(block);
		it->second = block;
	}

	// 3. 设置基本块的前驱和后继
	p = db.GetCartridge().GetData(start);
	for (auto block : blocks)
	{
		for (current = block->GetStartAddress(); current < block->GetEndAddress(); p += bytes)
		{
			instruction.Set(current, p);  // 构造指令对象
			const auto& entry = instruction.GetEntry();
			bytes = entry.length;
			current += bytes;  // 计算下一条指令的地址

			switch (entry.opcode)
			{
			case Opcode::Jmp:
				if (entry.addrMode == AddrMode::Absolute)
				{
					jumpAddr = instruction.GetOperandAddress();
					if (jumpAddr >= start && jumpAddr < end)
					{
						auto next = addrBlockMap[jumpAddr];
						block->AddSucc(jumpAddr);
						next->AddPred(block->GetStartAddress());
					}
					block->flag |= BBF_END_UNCOND;
					if (jumpAddr <= block->GetStartAddress())
					{
						block->flag |= BBF_JUMP_BEFOER;
						if (jumpAddr == block->GetStartAddress())
							block->flag |= BBF_JUMP_SELF;
					}
				}
				else
				{
					// 间接寻址相当于尾函数调用
					block->flag |= BBF_END_RETURN;
				}
				break;
			case Opcode::Bpl:
			case Opcode::Bmi:
			case Opcode::Bne:
			case Opcode::Beq:
			case Opcode::Bcc:
			case Opcode::Bcs:
			case Opcode::Bvc:
			case Opcode::Bvs:
				// 无条件跳转指令继续分析下一条
				jumpAddr = instruction.GetConditionalJumpAddress();
				if (jumpAddr >= start && jumpAddr < end)
				{
					auto next = addrBlockMap[jumpAddr];
					block->AddSucc(jumpAddr);
					next->AddPred(block->GetStartAddress());
				}
				if (current < end)
				{
					auto next = addrBlockMap[jumpAddr];
					block->AddSucc(jumpAddr);
					next->AddPred(block->GetStartAddress());
				}
				break;
			case Opcode::Rti:
			case Opcode::Rts:
				block->flag |= BBF_END_RETURN;
				break;
			default:
				if (current == block->GetEndAddress())
				{
					NesBasicBlock* next = addrBlockMap[current];
					if (next)
					{
						block->AddSucc(current);
						next->AddPred(block->GetStartAddress());
						block->flag |= BBF_END_NORMAL;
					}
				}
			}
		}
	}
	return blocks;
}

void NesBasicBlockParser::Reset()
{
	addrBlockMap.clear();
}

void NesBasicBlockParser::AddBlock(uint32_t start, uint32_t end)
{

}
