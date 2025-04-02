#include "stdafx.h"
#include "SubroutineRangeParser.h"
#include "NesDataBase.h"
using namespace Nes;

Sprintf<> s;


SubroutineRangeParser::SubroutineRangeParser(NesDataBase& db_) :
db(db_)
{

}

SubroutineRangeParser::~SubroutineRangeParser()
{
}

void SubroutineRangeParser::Parse()
{
	// 依次识别3个中断处理程序
	//AddSubroutineAddress(db.GetInterruptResetAddress());
	AddSubroutineAddress(db.GetInterruptNmiAddress());
	//AddSubroutineAddress(db.GetInterruptIrqAddress());

	while (!subAddrs.empty())
	{
		// 取出一个函数地址进行分析
		auto addr = *subAddrs.rbegin();
		subAddrs.pop_back();

		ParseSubroutine(addr);
	}

	int i = 0;
	for (auto& r : subroutines)
	{
		s.Format(_T("%d\t sub %04X - %04X\n"), i++, r.start, r.end);
		COUT << s.ToString();
	}
}

// 识别子程序范围的算法假设子程序内的指令都是连续存放的
// 如果跳转地址比函数开始地址小或者跳转地址不与函数地址连续
// 则认为是尾调用
void SubroutineRangeParser::ParseSubroutine(Address address)
{
	uint32_t maxAddress = 0x10000;
	auto ret = GetSubroutineOrNext(address);
	if (ret)
	{
		if (address == ret->start)
			return;  // 已经分析过了
		if (address < ret->start)
			maxAddress = ret->start;  // 最多到后面一个函数的开始地址
		else if (address < ret->end)
		{
			// 当前函数内联在另一个函数中，拆分后重新分析，因为可能有指令从
			// 拆分后的一个子程序跳转到另一个子程序
			COUT << s.Format(_T("拆分函数 %04X - %04X - %04X\n"), ret->start, address, ret->end);
			maxAddress = ret->end;
			ret->end = address;
			ParseSubroutineRange(ret->start, ret->end);
		}
	}
	auto range = ParseSubroutineRange(address, maxAddress);
	AddSubroutine(range.start, range.end);
}

void SubroutineRangeParser::AddSubroutine(Address start, Address end)
{
	auto it = std::lower_bound(subroutines.begin(), subroutines.end(), start,
		[](const SubroutineRange& subroutine, Address address) {
		return subroutine.start < address;
	});
	subroutines.insert(it, { start, end });
}

SubroutineRange* SubroutineRangeParser::GetSubroutineOrNext(Nes::Address address)
{
	// 查找第一个结束地址大于指定地址的子程序
	auto it = std::lower_bound(subroutines.begin(), subroutines.end(), address,
		[](const SubroutineRange& a, Nes::Address address){
		return a.end <= address;
	});
	// 如果该子程序包含指定地址，则返回它，否则返回空
	if (it == subroutines.end())
		return nullptr;
	return &subroutines[it - subroutines.begin()];
}

SubroutineRange SubroutineRangeParser::ParseSubroutineRange(uint32_t start, uint32_t end)
{
	COUT << s.Format(_T("\n===parse subroutine %04X===\n"), start);

	Instruction instruction;
	// 从卡带中获取函数开头指针
	const uint8_t* p = db.GetCartridge().GetData(start);
	std::unordered_set<uint32_t> jumpAddrs;  // 跳转指令跳转到的目标地址

	uint32_t current = start;  // 当前分析的指令地址
	uint32_t jumpAddr;
	int bytes;
	for (; current < end; p += bytes)
	{
		instruction.Set(current, p);  // 构造指令对象
		auto& entry = instruction.GetEntry();
		bytes = entry.length;
		current += bytes;  // 计算下一条指令的地址

		switch (entry.opcode)
		{
		case Opcode::Jmp:
			if (entry.addrMode == AddrMode::Absolute)
			{
				jumpAddr = instruction.GetOperandAddress();
				jumpAddrs.insert(jumpAddr);
			}
			// 间接寻址相当于尾函数调用
			// JMP 同 RTI，RTS 同样处理
		case Opcode::Rti:
		case Opcode::Rts: // 子程序结束
			if (jumpAddrs.find(current) != jumpAddrs.end())  // 下一条指令是跳转目标则继续分析
				continue;
			goto END;
		case Opcode::None:  // 非法指令
			if (jumpAddrs.find(current) != jumpAddrs.end())  // 下一条指令是跳转目标则继续分析
				continue;
			current -= bytes;  // 函数种不包含非法指令
			goto END;
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
			if (jumpAddr < start)
			{
				// 跳转到函数之前了，当作是尾调用处理
			}
			jumpAddrs.insert(jumpAddr);
			// 如果是
			break;
		case Opcode::Jsr:
			COUT << s.Format(_T("函数 %04X 调用 %04X\n"), start, instruction.GetOperandAddress());
			AddSubroutineAddress(instruction.GetOperandAddress());
			break;
		}
	}

END:
	// 跳转到范围外的地址，都认为是尾调用地址
	for (auto addr : jumpAddrs)
	{
		if (addr < start || addr >= current)
		{
			COUT << s.Format(_T("函数 %04X 调用 %04X\n"), start, addr);
			AddSubroutineAddress(addr);
		}
	}
	COUT << s.Format(_T("===parse subroutine end %04X - %04X===\n"), start, current);
	return{ start, current };
}
