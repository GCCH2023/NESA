#include "stdafx.h"
#include "NesBase.h"
using namespace Nes;

int Nes::GetInstructionBytesCount(AddrMode addrMode)
{
	static const int bytes[] = {1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 2, 2};
	return bytes[addrMode];
}

const TCHAR* Nes::ToString(Opcode opcode)
{
	static const TCHAR* names[] =
	{
		_T("None"),
		_T("Brk"), _T("Ora"), _T("Slo"), _T("Nop"), _T("Asl"), _T("Lsr"), _T("Php"),
		_T("Bpl"), _T("Bmi"),
		_T("Clc"), _T("Jsr"), _T("And"), _T("Bit"), _T("Rol"), _T("Rla"), _T("Plp"),
		_T("Sec"), _T("Rti"), _T("Eor"), _T("Sre"), _T("Pha"), _T("Jmp"),
		_T("Bvc"), _T("Cli"), _T("Rts"), _T("Adc"), _T("Ror"), _T("Rra"), _T("Pla"),
		_T("Bvs"), _T("Sei"), _T("Sta"), _T("Sax"), _T("Sty"), _T("Stx"), _T("Dey"), _T("Txa"),
		_T("Bcc"), _T("Tya"), _T("Txs"), _T("Lda"), _T("Ldx"), _T("Ldy"), _T("Clv"), _T("Tsx"),
		_T("Cpy"), _T("Cmp"), _T("Dcp"), _T("Dec"), _T("Iny"), _T("Dex"), _T("Bne"),
		_T("Cld"), _T("Cpx"), _T("Sbc"), _T("Isc"), _T("Inc"), _T("Inx"), _T("Beq"), _T("Sed"),
		_T("Lax"), _T("Tay"), _T("Tax"), _T("Bcs")
	};
	return names[(int)opcode];
}

const TCHAR* Nes::ToString(AddrMode addrMode)
{
	static const TCHAR* names[] =
	{
		_T("Implied"),
		_T("Accumulator"),
		_T("Immediate"),
		_T("ZeroPage"),
		_T("ZeroPageX"),
		_T("ZeroPageY"),
		_T("Relative"),
		_T("Absolute"),
		_T("AbsoluteX"),
		_T("AbsoluteY"),
		_T("Indirect"),
		_T("IndirectX"),
		_T("IndirectY"),
	};
	return names[addrMode];
}

const TCHAR* Nes::ToString(NesRegisters reg)
{
	static const TCHAR* names[] =
	{
		_T("A"), _T("X"), _T("Y"), _T("P"), _T("SP")
	};
	return names[reg];
}

// 操作码类缩写
// 命名规则：是什么指令，影响了什么寄存器或内存，影响了什么标志位
// R: read, W: write, A: affect
enum OpcodeClass
{
	//AffectPC_BI = OpKind::RWpc | OpKind::BI,
	LogicRmRWa_NZ = OpKind::Logical | OpKind::ReadM | OpKind::RWa | OpKind::NZ,
	RpRWsp = OpKind::RWsp | OpKind::RWp,
	RspWp = OpKind::ReadSP | OpKind::WriteP,
	RaRWsp = OpKind::ReadA | OpKind::RWsp,
	RspWa_NZ = OpKind::ReadSP | OpKind::WriteA | OpKind::NZ,
	ArithAm_NZC = OpKind::Arithmetic | OpKind::RWm | OpKind::NZC,
	ArithRmRa_NZC = OpKind::Arithmetic | OpKind::ReadM | OpKind::ReadA | OpKind::NZC,
	ArithRmRx_NZC = OpKind::Arithmetic | OpKind::ReadM | OpKind::ReadX | OpKind::NZC,
	ArithRmRy_NZC = OpKind::Arithmetic | OpKind::ReadM | OpKind::ReadY | OpKind::NZC,
	ArithRWmRp_NZC = OpKind::Arithmetic | OpKind::RWm | OpKind::ReadP | OpKind::NZC,
	ArithRWm_NZC = OpKind::Arithmetic | OpKind::RWm | OpKind::NZC,
	ArithRWmRa_NZC = OpKind::Arithmetic | OpKind::RWm | OpKind::ReadA | OpKind::NZC,
	ArithRmAa_NVZC = OpKind::Arithmetic | OpKind::ReadM | OpKind::RWa | OpKind::NVZC,
	ArithRWmRWa_NVZC = OpKind::Arithmetic | OpKind::RWm | OpKind::RWa | OpKind::NVZC,
	ArithRmRWaRp_NVZC = OpKind::Arithmetic | OpKind::ReadM | OpKind::RWa | OpKind::ReadP | OpKind::NVZC,
	ArithRmRWa_NZ = OpKind::Arithmetic | OpKind::ReadM | OpKind::RWa | OpKind::NZ,
	CondJumpRp = OpKind::ConditionalJump | OpKind::ReadP,
	RmRa_NVZ = OpKind::ReadM | OpKind::ReadA | OpKind::NVZ,
	ArithRWm_NZ = OpKind::Arithmetic | OpKind::RWm | OpKind::NZ,
	ArithRWx_NZ = OpKind::Arithmetic | OpKind::RWx | OpKind::NZ,
	ArithRWy_NZ = OpKind::Arithmetic | OpKind::RWy | OpKind::NZ,
	JumpRmWpc = OpKind::UnconditionalJump | OpKind::ReadM | OpKind::WritePC,
	CallRmRWpcWsp = OpKind::Call | OpKind::ReadM | OpKind::RWpc | OpKind::WriteSP,
	RetRspWpc = OpKind::Return | OpKind::ReadSP | OpKind::WritePC,
	MoveRmWa_NZ = OpKind::Move | OpKind::ReadM | OpKind::WriteA | OpKind::NZ,
	MoveRmWx_NZ = OpKind::Move | OpKind::ReadM | OpKind::WriteX | OpKind::NZ,
	MoveRmWy_NZ = OpKind::Move | OpKind::ReadM | OpKind::WriteY | OpKind::NZ,
	MoveRaWm = OpKind::Move | OpKind::ReadA | OpKind::WriteM,
	MoveRxWm = OpKind::Move | OpKind::ReadX | OpKind::WriteM,
	MoveRyWm = OpKind::Move | OpKind::ReadY | OpKind::WriteM,
	MoveRaWx_NZ = OpKind::Move | OpKind::ReadA | OpKind::WriteX | OpKind::NZ,
	MoveRaWy_NZ = OpKind::Move | OpKind::ReadA | OpKind::WriteY | OpKind::NZ,
	MoveRspWx_NZ = OpKind::Move | OpKind::ReadSP | OpKind::WriteX | OpKind::NZ,
	MoveRxWa_NZ = OpKind::Move | OpKind::ReadX | OpKind::WriteA | OpKind::NZ,
	MoveRyWa_NZ = OpKind::Move | OpKind::ReadY | OpKind::WriteA | OpKind::NZ,
	IllegalMoveRmWaWx_NZ = OpKind::Move | OpKind::ReadM | OpKind::WriteA | OpKind::WriteX | OpKind::NZ,
	MoveRxWsp = OpKind::Move | OpKind::ReadX | OpKind::WriteSP,
	IllegalAmApc_NZC = OpKind::Illegal | OpKind::RWm | OpKind::RWpc | OpKind::NZC,
	IllegalRWmRWa_NZC = OpKind::Illegal | OpKind::RWm | OpKind::RWa | OpKind::NZC,
	IllegalRWmRWa_NVZC = OpKind::Illegal | OpKind::RWm | OpKind::RWa | OpKind::NVZC,
	IllegalLogicRaRxWm = OpKind::Logical | OpKind::ReadA | OpKind::ReadX | OpKind::WriteM,
};

OpcodeEntry::OpcodeEntry(Opcode opcode_, AddrMode addrMode, int cycles, bool extraCycle /*= false*/) :
opcode(opcode_),
addrMode(addrMode)
{
	switch (addrMode)
	{
	case AddrMode::Implied:
	case AddrMode::Accumulator:
		this->length = 1;
		break;
	case AddrMode::Immediate:
	case AddrMode::ZeroPage:
	case AddrMode::ZeroPageX:
	case AddrMode::ZeroPageY:
	case AddrMode::Relative:
	case AddrMode::IndirectX:
	case AddrMode::IndirectY:
		this->length = 2;
		break;
	case AddrMode::Absolute:
	case AddrMode::AbsoluteX:
	case AddrMode::AbsoluteY:
	case AddrMode::Indirect:
		this->length = 3;
		break;
	default:
		throw Exception(_T("无效的寻址模式"));
	}
	this->cycles = cycles;
	switch (opcode)
	{
	case Opcode::Adc: this->kind = ArithRmAa_NVZC; break;
	case Opcode::And: this->kind = LogicRmRWa_NZ; break;
	case Opcode::Asl: this->kind = ArithAm_NZC; break;
	case Opcode::Bcc: this->kind = CondJumpRp; break;
	case Opcode::Bcs: this->kind = CondJumpRp; break;
	case Opcode::Beq: this->kind = CondJumpRp; break;
	case Opcode::Bit: this->kind = RmRa_NVZ; break;
	case Opcode::Bmi: this->kind = CondJumpRp; break;
	case Opcode::Bne: this->kind = CondJumpRp; break;
	case Opcode::Bpl: this->kind = CondJumpRp; break;
	case Opcode::Brk: this->kind = OpKind::None; break;    // 特殊处理
	case Opcode::Bvc: this->kind = CondJumpRp; break;
	case Opcode::Bvs: this->kind = CondJumpRp; break;
	case Opcode::Clc: this->kind = OpKind::Write_C; break;
	case Opcode::Cld: this->kind = OpKind::Write_D3; break;
	case Opcode::Cli: this->kind = OpKind::Write_I; break;
	case Opcode::Clv: this->kind = OpKind::Write_V; break;
	case Opcode::Cmp: this->kind = ArithRmRa_NZC; break;
	case Opcode::Cpx: this->kind = ArithRmRx_NZC; break;
	case Opcode::Cpy: this->kind = ArithRmRy_NZC; break;
	case Opcode::Dec: this->kind = ArithRWm_NZ; break;
	case Opcode::Dex: this->kind = ArithRWx_NZ; break;
	case Opcode::Dey: this->kind = ArithRWy_NZ; break;
	case Opcode::Eor: this->kind = LogicRmRWa_NZ; break;
	case Opcode::Inc: this->kind = ArithRWm_NZ; break;
	case Opcode::Inx: this->kind = ArithRWx_NZ; break;
	case Opcode::Iny: this->kind = ArithRWy_NZ; break;
	case Opcode::Jmp: this->kind = JumpRmWpc; break;
	case Opcode::Jsr: this->kind = CallRmRWpcWsp; break;
	case Opcode::Lda: this->kind = MoveRmWa_NZ; break;
	case Opcode::Ldx: this->kind = MoveRmWx_NZ; break;
	case Opcode::Ldy: this->kind = MoveRmWy_NZ; break;
	case Opcode::Lsr: this->kind = ArithRWm_NZC; break;
	case Opcode::Nop: this->kind = OpKind::None; break;
	case Opcode::Ora: this->kind = LogicRmRWa_NZ; break;
	case Opcode::Pha: this->kind = RaRWsp; break;
	case Opcode::Php: this->kind = RpRWsp; break;
	case Opcode::Pla: this->kind = RspWa_NZ; break;
	case Opcode::Plp: this->kind = RspWp; break;
	case Opcode::Rol: this->kind = ArithRWmRp_NZC; break;
	case Opcode::Ror: this->kind = ArithRWmRp_NZC; break;
	case Opcode::Rti: this->kind = OpKind::Return; break; // 特殊
	case Opcode::Rts: this->kind = RetRspWpc; break;
	case Opcode::Sbc: this->kind = ArithRmRWaRp_NVZC; break;
	case Opcode::Sec: this->kind = OpKind::Write_C; break;
	case Opcode::Sed: this->kind = OpKind::Write_D3; break;
	case Opcode::Sei: this->kind = OpKind::Write_I; break;
	case Opcode::Sta: this->kind = MoveRaWm; break;
	case Opcode::Stx: this->kind = MoveRxWm; break;
	case Opcode::Sty: this->kind = MoveRyWm; break;
	case Opcode::Tax: this->kind = MoveRaWx_NZ; break;
	case Opcode::Tay: this->kind = MoveRaWy_NZ; break;
	case Opcode::Tsx: this->kind = MoveRspWx_NZ; break;
	case Opcode::Txa: this->kind = MoveRxWa_NZ; break;
	case Opcode::Txs: this->kind = MoveRxWsp; break;
	case Opcode::Tya: this->kind = MoveRyWa_NZ; break;
	case Opcode::Dcp: this->kind = ArithRWmRa_NZC; break;
	case Opcode::Isc: this->kind = ArithRWmRWa_NVZC; break;
	case Opcode::Lax: this->kind = IllegalMoveRmWaWx_NZ; break;
	case Opcode::Rla: this->kind = IllegalRWmRWa_NZC; break;
	case Opcode::Rra: this->kind = IllegalRWmRWa_NVZC; break;
	case Opcode::Sax: this->kind = IllegalLogicRaRxWm; break;
	case Opcode::Slo: this->kind = IllegalAmApc_NZC; break;
	case Opcode::Sre: this->kind = IllegalRWmRWa_NZC; break;
	case Opcode::None:
		this->kind = OpKind::None;
		break;
	default:
		throw Exception(_T("无效的操作码"));
	}
	if (extraCycle)
		this->kind |= OpKind::ExtraCycle;
}

// 寻址模式缩写
static AddrMode am_def = AddrMode::Implied;  // 没有寻址模式
static AddrMode am_izx = AddrMode::IndirectX;
static AddrMode am_izy = AddrMode::IndirectY;
static AddrMode am_zp = AddrMode::ZeroPage;
static AddrMode am_zpx = AddrMode::ZeroPageX;
static AddrMode am_zpy = AddrMode::ZeroPageY;
static AddrMode am_imm = AddrMode::Immediate;
static AddrMode am_abs = AddrMode::Absolute;
static AddrMode am_abx = AddrMode::AbsoluteX;
static AddrMode am_aby = AddrMode::AbsoluteY;
static AddrMode am_rel = AddrMode::Relative;
static AddrMode am_acc = AddrMode::Accumulator;

const OpcodeEntry OpcodeEntry::None(Opcode::None, am_def, 0);

static const OpcodeEntry opentrys[] =
{
	OpcodeEntry(Opcode::Brk, am_def, 7),  // 00
	OpcodeEntry(Opcode::Ora, am_izx, 6),  // 01
	OpcodeEntry::None, // 02
	OpcodeEntry(Opcode::Slo, am_izx, 8),  // 03
	OpcodeEntry(Opcode::Nop, am_zp, 3),  // 04
	OpcodeEntry(Opcode::Ora, am_zp, 3),  // 05
	OpcodeEntry(Opcode::Asl, am_zp, 5), // 06
	OpcodeEntry(Opcode::Slo, am_zp, 5), // 07
	OpcodeEntry(Opcode::Php, am_def, 3), // 08
	OpcodeEntry(Opcode::Ora, am_imm, 2), // 09
	OpcodeEntry(Opcode::Asl, am_acc, 2), // 0A
	OpcodeEntry::None, // 0B
	OpcodeEntry(Opcode::Nop, am_abs, 4), // 0C
	OpcodeEntry(Opcode::Ora, am_abs, 4), // 0D
	OpcodeEntry(Opcode::Asl, am_abs, 6), // 0E
	OpcodeEntry(Opcode::Slo, am_abs, 6), // 0F
	OpcodeEntry(Opcode::Bpl, am_rel, 2, true), // 10
	OpcodeEntry(Opcode::Ora, am_izy, 5, true), // 11
	OpcodeEntry::None, // 12
	OpcodeEntry(Opcode::Slo, am_izy, 8), // 13
	OpcodeEntry(Opcode::Nop, am_zpx, 4), // 14
	OpcodeEntry(Opcode::Ora, am_zpx, 4), // 15
	OpcodeEntry(Opcode::Asl, am_zpx, 6), // 16
	OpcodeEntry(Opcode::Slo, am_zpx, 6), // 17
	OpcodeEntry(Opcode::Clc, am_def, 2), // 18
	OpcodeEntry(Opcode::Ora, am_aby, 4, true), // 19
	OpcodeEntry(Opcode::Nop, am_def, 2), // 1A
	OpcodeEntry(Opcode::Slo, am_aby, 7), // 1B
	OpcodeEntry(Opcode::Nop, am_abx, 4, true), // 1C
	OpcodeEntry(Opcode::Ora, am_abx, 4, true), // 1D
	OpcodeEntry(Opcode::Asl, am_abx, 7), // 1E
	OpcodeEntry(Opcode::Slo, am_abx, 7), // 1F
	OpcodeEntry(Opcode::Jsr, am_abs, 6), // 20
	OpcodeEntry(Opcode::And, am_izx, 6), // 21
	OpcodeEntry::None, // 22
	OpcodeEntry(Opcode::Rla, am_izx, 8), // 23
	OpcodeEntry(Opcode::Bit, am_zp, 3), // 24
	OpcodeEntry(Opcode::And, am_zp, 3), // 25
	OpcodeEntry(Opcode::Rol, am_zp, 5), // 26
	OpcodeEntry(Opcode::Rla, am_zp, 5), // 27
	OpcodeEntry(Opcode::Plp, am_def, 5), // 28
	OpcodeEntry(Opcode::And, am_imm, 2), // 29
	OpcodeEntry(Opcode::Rol, am_acc, 2), // 2A
	OpcodeEntry::None, // 2B
	OpcodeEntry(Opcode::Bit, am_abs, 4), // 2C
	OpcodeEntry(Opcode::And, am_abs, 4), // 2D
	OpcodeEntry(Opcode::Rol, am_abs, 6), // 2E
	OpcodeEntry(Opcode::Rla, am_abs, 6), // 2F
	OpcodeEntry(Opcode::Bmi, am_rel, 2, true), // 30
	OpcodeEntry(Opcode::And, am_izy, 5, true), // 31
	OpcodeEntry::None, // 32
	OpcodeEntry(Opcode::Rla, am_izy, 8), // 33
	OpcodeEntry(Opcode::Nop, am_zpx, 4), // 34
	OpcodeEntry(Opcode::And, am_zpx, 4), // 35
	OpcodeEntry(Opcode::Rol, am_zpx, 6), // 36
	OpcodeEntry(Opcode::Rla, am_zpx, 6), // 37
	OpcodeEntry(Opcode::Sec, am_def, 2), // 38
	OpcodeEntry(Opcode::And, am_aby, 4, true), // 39
	OpcodeEntry(Opcode::Nop, am_def, 2), // 3A
	OpcodeEntry(Opcode::Rla, am_aby, 7), // 3B
	OpcodeEntry(Opcode::Nop, am_abx, 4, true), // 3C
	OpcodeEntry(Opcode::And, am_abx, 4, true), // 3D
	OpcodeEntry(Opcode::Rol, am_abx, 7), // 3E
	OpcodeEntry(Opcode::Rla, am_abx, 7), // 3F
	OpcodeEntry(Opcode::Rti, am_def, 6), // 40
	OpcodeEntry(Opcode::Eor, am_izx, 6), // 41
	OpcodeEntry::None, // 42
	OpcodeEntry(Opcode::Sre, am_izx, 8), // 43
	OpcodeEntry(Opcode::Nop, am_zp, 3), // 44
	OpcodeEntry(Opcode::Eor, am_zp, 3), // 45
	OpcodeEntry(Opcode::Lsr, am_zp, 5), // 46
	OpcodeEntry(Opcode::Sre, am_zp, 5), // 47
	OpcodeEntry(Opcode::Pha, am_def, 3), // 72, 48H
	OpcodeEntry(Opcode::Eor, am_imm, 2), // 73, 49H
	OpcodeEntry(Opcode::Lsr, am_acc, 2), // 4A
	OpcodeEntry::None, // 4B
	OpcodeEntry(Opcode::Jmp, am_abs, 3), // 4C
	OpcodeEntry(Opcode::Eor, am_abs, 4), // 4D
	OpcodeEntry(Opcode::Lsr, am_abs, 6), // 4E
	OpcodeEntry(Opcode::Sre, am_abs, 6), // 4F
	OpcodeEntry(Opcode::Bvc, am_rel, 2, true), // 50
	OpcodeEntry(Opcode::Eor, am_izy, 5, true), // 51
	OpcodeEntry::None, // 52
	OpcodeEntry(Opcode::Sre, am_izy, 8), // 53
	OpcodeEntry(Opcode::Nop, am_zpx, 4), // 54
	OpcodeEntry(Opcode::Eor, am_zpx, 4), // 55
	OpcodeEntry(Opcode::Lsr, am_zpx, 6), // 56
	OpcodeEntry(Opcode::Sre, am_zpx, 6), // 57
	OpcodeEntry(Opcode::Cli, am_def, 2), // 58
	OpcodeEntry(Opcode::Eor, am_aby, 4, true), // 59
	OpcodeEntry(Opcode::Nop, am_def, 2), // 5A
	OpcodeEntry(Opcode::Sre, am_aby, 7), // 5B
	OpcodeEntry(Opcode::Nop, am_abx, 4, true), // 5C
	OpcodeEntry(Opcode::Eor, am_abx, 4, true), // 5D
	OpcodeEntry(Opcode::Lsr, am_abx, 7), // 5E
	OpcodeEntry(Opcode::Sre, am_abx, 7), // 5F
	OpcodeEntry(Opcode::Rts, am_def, 6), // 60
	OpcodeEntry(Opcode::Adc, am_izx, 6), // 61
	OpcodeEntry::None, // 63
	OpcodeEntry(Opcode::Rra, am_izx, 8), // 63
	OpcodeEntry(Opcode::Nop, am_zp, 3), // 64
	OpcodeEntry(Opcode::Adc, am_zp, 3), // 65
	OpcodeEntry(Opcode::Ror, am_zp, 5), // 66
	OpcodeEntry(Opcode::Rra, am_zp, 5), // 67
	OpcodeEntry(Opcode::Pla, am_def, 4), // 68
	OpcodeEntry(Opcode::Adc, am_imm, 2), // 69
	OpcodeEntry(Opcode::Ror, am_acc, 2), // 6A
	OpcodeEntry::None, // 6B
	OpcodeEntry(Opcode::Jmp, AddrMode::Indirect, 5), // 6C
	OpcodeEntry(Opcode::Adc, am_abs, 4), // 6D
	OpcodeEntry(Opcode::Ror, am_abs, 6), // 6E
	OpcodeEntry(Opcode::Rra, am_abs, 6), // 6F
	OpcodeEntry(Opcode::Bvs, am_rel, 2), // 70
	OpcodeEntry(Opcode::Adc, am_izy, 5), // 71
	OpcodeEntry::None, // 72
	OpcodeEntry(Opcode::Rra, am_izy, 8), // 73
	OpcodeEntry(Opcode::Nop, am_zpx, 4), // 74
	OpcodeEntry(Opcode::Adc, am_zpx, 4), // 75
	OpcodeEntry(Opcode::Ror, am_zpx, 6), // 76
	OpcodeEntry(Opcode::Rra, am_zpx, 6), // 77
	OpcodeEntry(Opcode::Sei, am_def, 2), // 78
	OpcodeEntry(Opcode::Adc, am_aby, 4), // 79
	OpcodeEntry(Opcode::Nop, am_def, 2), // 7A
	OpcodeEntry(Opcode::Rra, am_aby, 7), // 7B
	OpcodeEntry(Opcode::Nop, am_abx, 4), // 7C
	OpcodeEntry(Opcode::Adc, am_abx, 4), // 7D
	OpcodeEntry(Opcode::Ror, am_abx, 7), // 7E
	OpcodeEntry(Opcode::Rra, am_abx, 7), // 7F
	OpcodeEntry(Opcode::Nop, am_imm, 2), // 80
	OpcodeEntry(Opcode::Sta, am_izx, 6), // 81
	OpcodeEntry(Opcode::Nop, am_imm, 2), // 82
	OpcodeEntry(Opcode::Sax, am_izx, 6), // 83
	OpcodeEntry(Opcode::Sty, am_zp, 3), // 84
	OpcodeEntry(Opcode::Sta, am_zp, 3), // 85
	OpcodeEntry(Opcode::Stx, am_zp, 3), // 86
	OpcodeEntry(Opcode::Sax, am_zp, 3), // 87
	OpcodeEntry(Opcode::Dey, am_def, 2), // 88
	OpcodeEntry::None, // 89
	OpcodeEntry(Opcode::Txa, am_def, 2), // 8A
	OpcodeEntry::None, // 8B
	OpcodeEntry(Opcode::Sty, am_abs, 4), // 8C
	OpcodeEntry(Opcode::Sta, am_abs, 4), // 8D
	OpcodeEntry(Opcode::Stx, am_abs, 4), // 8E
	OpcodeEntry(Opcode::Sax, am_abs, 4), // 8F
	OpcodeEntry(Opcode::Bcc, am_rel, 2), // 90
	OpcodeEntry(Opcode::Sta, am_izy, 6), // 91
	OpcodeEntry::None, // 92
	OpcodeEntry::None, // 93
	OpcodeEntry(Opcode::Sty, am_zpx, 4), // 94
	OpcodeEntry(Opcode::Sta, am_zpx, 4), // 95
	OpcodeEntry(Opcode::Stx, am_zpy, 4), // 96
	OpcodeEntry(Opcode::Sax, am_zpy, 4), // 97
	OpcodeEntry(Opcode::Tya, am_def, 2), // 98
	OpcodeEntry(Opcode::Sta, am_aby, 5), // 99
	OpcodeEntry(Opcode::Txs, am_def, 2), // 9A
	OpcodeEntry::None, // 9B
	OpcodeEntry::None, // 9C
	OpcodeEntry(Opcode::Sta, am_abx, 5), // 9D
	OpcodeEntry::None, // 9E
	OpcodeEntry::None, // 9F
	OpcodeEntry(Opcode::Ldy, am_imm, 2), // A0
	OpcodeEntry(Opcode::Lda, am_izx, 6), // A1
	OpcodeEntry(Opcode::Ldx, am_imm, 2), // A2
	OpcodeEntry(Opcode::Lax, am_izx, 6), // A3
	OpcodeEntry(Opcode::Ldy, am_zp, 3), // A4
	OpcodeEntry(Opcode::Lda, am_zp, 3), // A5
	OpcodeEntry(Opcode::Ldx, am_zp, 3), // A6
	OpcodeEntry(Opcode::Lax, am_zp, 3), // A7
	OpcodeEntry(Opcode::Tay, am_def, 2), // A8
	OpcodeEntry(Opcode::Lda, am_imm, 2), // A9
	OpcodeEntry(Opcode::Tax, am_def, 2), // AA
	OpcodeEntry::None, // AB
	OpcodeEntry(Opcode::Ldy, am_abs, 4), // AC
	OpcodeEntry(Opcode::Lda, am_abs, 4), // AD
	OpcodeEntry(Opcode::Ldx, am_abs, 4), // AE
	OpcodeEntry(Opcode::Lax, am_abs, 4), // AF
	OpcodeEntry(Opcode::Bcs, am_rel, 2), // B0
	OpcodeEntry(Opcode::Lda, am_izy, 5), // B1
	OpcodeEntry::None, // B2
	OpcodeEntry(Opcode::Lax, am_izy, 5), // B3
	OpcodeEntry(Opcode::Ldy, am_zpx, 4), // B4
	OpcodeEntry(Opcode::Lda, am_zpx, 4), // B5
	OpcodeEntry(Opcode::Ldx, am_zpy, 4), // B6
	OpcodeEntry(Opcode::Lax, am_zpy, 4), // B7
	OpcodeEntry(Opcode::Clv, am_def, 2), // B8
	OpcodeEntry(Opcode::Lda, am_aby, 4), // B9
	OpcodeEntry(Opcode::Tsx, am_def, 2), // BA
	OpcodeEntry::None, // BB
	OpcodeEntry(Opcode::Ldy, am_abx, 4), // BC
	OpcodeEntry(Opcode::Lda, am_abx, 4), // BD
	OpcodeEntry(Opcode::Ldx, am_aby, 4), // BE
	OpcodeEntry(Opcode::Lax, am_aby, 4), // BF
	OpcodeEntry(Opcode::Cpy, am_imm, 2), // C0
	OpcodeEntry(Opcode::Cmp, am_izx, 6), // C1
	OpcodeEntry::None, // C2
	OpcodeEntry(Opcode::Dcp, am_izx, 8), // C3
	OpcodeEntry(Opcode::Cpy, am_zp, 3), // C4
	OpcodeEntry(Opcode::Cmp, am_zp, 3), // C5
	OpcodeEntry(Opcode::Dec, am_zp, 5), // C6
	OpcodeEntry(Opcode::Dcp, am_zp, 5), // C7
	OpcodeEntry(Opcode::Iny, am_def, 2), // C8
	OpcodeEntry(Opcode::Cmp, am_imm, 2), // C9
	OpcodeEntry(Opcode::Dex, am_def, 2), // CA
	OpcodeEntry::None, // CB
	OpcodeEntry(Opcode::Cpy, am_abs, 4), // CC
	OpcodeEntry(Opcode::Cmp, am_abs, 4), // CD
	OpcodeEntry(Opcode::Dec, am_abs, 6), // CE
	OpcodeEntry(Opcode::Dcp, am_abs, 6), // CF
	OpcodeEntry(Opcode::Bne, am_rel, 2), // D0
	OpcodeEntry(Opcode::Cmp, am_izy, 5), // D1
	OpcodeEntry::None, // D2
	OpcodeEntry(Opcode::Dcp, am_izy, 8), // D3
	OpcodeEntry(Opcode::Nop, am_zpx, 4), // D4
	OpcodeEntry(Opcode::Cmp, am_zpx, 4), // D5
	OpcodeEntry(Opcode::Dec, am_zpx, 6), // D6
	OpcodeEntry(Opcode::Dcp, am_zpx, 6), // D7
	OpcodeEntry(Opcode::Cld, am_def, 2), // D8
	OpcodeEntry(Opcode::Cmp, am_aby, 4), // D9
	OpcodeEntry(Opcode::Nop, am_def, 2), // DA
	OpcodeEntry(Opcode::Dcp, am_aby, 7), // DB
	OpcodeEntry(Opcode::Nop, am_abx, 4), // DC
	OpcodeEntry(Opcode::Cmp, am_abx, 4), // DD
	OpcodeEntry(Opcode::Dec, am_abx, 7), // DE
	OpcodeEntry(Opcode::Dcp, am_abx, 7), // DF
	OpcodeEntry(Opcode::Cpx, am_imm, 2), // E0
	OpcodeEntry(Opcode::Sbc, am_izx, 6), // E1
	OpcodeEntry::None, // E2
	OpcodeEntry(Opcode::Isc, am_izx, 8), // E3
	OpcodeEntry(Opcode::Cpx, am_zp, 3), // E4
	OpcodeEntry(Opcode::Sbc, am_zp, 3), // E5
	OpcodeEntry(Opcode::Inc, am_zp, 5), // E6
	OpcodeEntry(Opcode::Isc, am_zp, 5), // E7
	OpcodeEntry(Opcode::Inx, am_def, 2), // E8
	OpcodeEntry(Opcode::Sbc, am_imm, 2), // E9
	OpcodeEntry(Opcode::Nop, am_def, 2), // EA
	OpcodeEntry(Opcode::Sbc, am_imm, 2), // EB
	OpcodeEntry(Opcode::Cpx, am_abs, 4), // EC
	OpcodeEntry(Opcode::Sbc, am_abs, 4), // ED
	OpcodeEntry(Opcode::Inc, am_abs, 6), // EE
	OpcodeEntry(Opcode::Isc, am_abs, 6), // EF
	OpcodeEntry(Opcode::Beq, am_rel, 2), // F0
	OpcodeEntry(Opcode::Sbc, am_izy, 5), // F1
	OpcodeEntry::None, // F2
	OpcodeEntry(Opcode::Isc, am_izy, 8), // F3
	OpcodeEntry(Opcode::Nop, am_zpx, 4), // F4
	OpcodeEntry(Opcode::Sbc, am_zpx, 4), // F5
	OpcodeEntry(Opcode::Inc, am_zpx, 6), // F6
	OpcodeEntry(Opcode::Isc, am_zpx, 6), // F7
	OpcodeEntry(Opcode::Sed, am_def, 2), // F8
	OpcodeEntry(Opcode::Sbc, am_aby, 4), // F9
	OpcodeEntry(Opcode::Nop, am_def, 2), // FA
	OpcodeEntry(Opcode::Isc, am_aby, 7), // FB
	OpcodeEntry(Opcode::Nop, am_abx, 4), // FC
	OpcodeEntry(Opcode::Sbc, am_abx, 4), // FD
	OpcodeEntry(Opcode::Inc, am_abx, 7), // FE
	OpcodeEntry(Opcode::Isc, am_abx, 7), // FF
};

const OpcodeEntry& Nes::GetOpcodeEntry(int code)
{
	return opentrys[code & 255];
}

const TCHAR* Nes::ToString(PPURegister value)
{
	static const TCHAR* names[] =
	{
		_T("PPU_CTRL"),
		_T("PPU_MASK"),
		_T("PPU_STATUS"),
		_T("OAM_ADDR"),
		_T("OAM_DATA"),
		_T("PPU_SCROLL"),
		_T("PPU_ADDR"),
		_T("PPU_DATA"),
	};
	return names[value & 7];
}
