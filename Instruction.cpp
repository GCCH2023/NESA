#include "stdafx.h"
#include "Instruction.h"
using namespace Nes;

const Nes::OpcodeEntry& Instruction::GetEntry() const
{
	return GetOpcodeEntry(data & 0xFF);
}

Nes::Opcode Instruction::GetOpcode() const
{
	return GetOpcodeEntry(data & 0xFF).opcode;
}

int Instruction::GetLength() const
{
	AddrMode addrMode = GetOpcodeEntry(data & 0xFF).addrMode;
	return GetInstructionBytesCount(addrMode);
}



const TCHAR* FormatInstruction(const Instruction& instruction)
{
	const OpcodeEntry& entry = GetOpcodeEntry(instruction.data & 0xFF);
	static TCHAR buffer[128];
	TCHAR* p = buffer;
	if (entry.opcode == Opcode::Jmp)
	{
		if (entry.addrMode == Absolute)
		{
			_stprintf_s(buffer, 128, _T("%s %04X"),
				ToString(entry.opcode), instruction.GetOperandAddress());
		}
		else if (entry.addrMode == Indirect)
		{
			_stprintf_s(buffer, 128, _T("%s [%04X]"),
				ToString(entry.opcode), instruction.GetOperandAddress());
		}
		return buffer;
	}
	switch (entry.addrMode)
	{
	case AddrMode::Implied:
		p += _stprintf_s(p, 128 - (p - buffer), _T("%s"), ToString(entry.opcode));
		break;
	case AddrMode::Accumulator:
		p += _stprintf_s(p, 128 - (p - buffer), _T("%s"), ToString(entry.opcode));
		break;
	case AddrMode::Immediate:
		p += _stprintf_s(p, 128 - (p - buffer), _T("%s #%02X"),
			ToString(entry.opcode), instruction.GetByte());
		break;
	case AddrMode::ZeroPage:
		p += _stprintf_s(p, 128 - (p - buffer), _T("%s [%02X]"), ToString(entry.opcode), instruction.GetByte());
		break;
	case AddrMode::ZeroPageX:
		p += _stprintf_s(p, 128 - (p - buffer), _T("%s [X + %02X]"), ToString(entry.opcode), instruction.GetByte());
		break;
	case AddrMode::ZeroPageY:
		p += _stprintf_s(p, 128 - (p - buffer), _T("%s [Y + %02X]"), ToString(entry.opcode), instruction.GetByte());
		break;
	case AddrMode::Relative:
		p += _stprintf_s(p, 128 - (p - buffer), _T("%s %04X"), ToString(entry.opcode), instruction.GetConditionalJumpAddress());
		break;
	case AddrMode::Absolute:
		p += _stprintf_s(p, 128 - (p - buffer), _T("%s [%04X]"),
			ToString(entry.opcode), instruction.GetOperandAddress());
		break;
	case AddrMode::AbsoluteX:
		p += _stprintf_s(p, 128 - (p - buffer), _T("%s [X + %04X]"), ToString(entry.opcode), instruction.GetOperandAddress());
		break;
	case AddrMode::AbsoluteY:
		p += _stprintf_s(p, 128 - (p - buffer), _T("%s [Y + %04X]"), ToString(entry.opcode), instruction.GetOperandAddress());
		break;
	case AddrMode::Indirect:
	{
							   int address = instruction.GetOperandAddress();
							   p += _stprintf_s(p, 128 - (p - buffer), _T("%s [%04X]"), ToString(entry.opcode), address);
	}
		break;
	case AddrMode::IndirectX:
	{

								uint8_t v = instruction.GetByte();
								p += _stprintf_s(p, 128 - (p - buffer), _T("%s [[X + %02X]]"), ToString(entry.opcode), v);
	}
		break;
	case AddrMode::IndirectY:
	{
								uint8_t v = instruction.GetByte();
								p += _stprintf_s(p, 128 - (p - buffer), _T("%s [Y + [%02X]]"), ToString(entry.opcode), v);
	}
		break;
	default:
		throw Exception(_T("格式化NES指令: 无效的寻址方式"));
	}
	return buffer;
}
