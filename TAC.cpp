#include "stdafx.h"
#include "TAC.h"
using namespace Nes;

TACOperand RegisterP(TACOperand::REGISTER | NesRegisters::P);
TACOperand RegisterA(TACOperand::REGISTER | NesRegisters::A);
TACOperand RegisterX(TACOperand::REGISTER | NesRegisters::X);
TACOperand RegisterY(TACOperand::REGISTER | NesRegisters::Y);
TACOperand RegisterSP(TACOperand::REGISTER | NesRegisters::SP);


TAC::TAC(TACOperator op_, TACOperand z_, TACOperand x_, TACOperand y_) :
op(op_),
x(x_),
y(y_),
z(z_)
{

}


TAC::TAC(TACOperator op_, TACOperand z_, TACOperand x_) :
op(op_),
x(x_),
z(z_)
{

}

TAC::TAC(TACOperator op_) :
op(op_)
{

}

TAC::TAC(TACOperator op_, TACOperand z_):
op(op_),
z(z_)
{

}

const TCHAR* ToString(TACOperator op)
{
	static const TCHAR* names[] =
	{
		_T("NOP"),
		_T("ADD"),
		_T("SUB"),
		_T("ASSIGN"),
		_T("BOR"),
		_T("BAND"),
		_T("BNOT"),
		_T("XOR"),
		_T("IFGREAT"),
		_T("IFGEQ"),
		_T("IFLESS"),
		_T("IFLEQ"),
		_T("IFNEQ"),
		_T("IFEQ"),
		_T("INDEX"),
		_T("REF"),
		_T("ARG"),
		_T("CALL"),
		_T("GOTO"),
		_T("RETURN"),
		_T("SHR"),
		_T("SHL"),
		_T("ROR"),
		_T("ROL"),
		_T("PUSH"),
		_T("POP"),

		_T("BREAK"),
		_T("BIT"),
	};
	return names[(int)op];
}

OStream& operator<<(OStream& os, const TACOperand& obj)
{
	switch (obj.GetKind())
	{
	case TACOperand::INTEGER:
	{
								  char buffer[16];
								  if (obj.IsTemp())
									  sprintf_s(buffer, "temp%d", obj.GetValue());
								  else
									  sprintf_s(buffer, "0x%02X", obj.GetValue());
								  os << buffer;
								  break;
	}
	case TACOperand::MEMORY:
	{
								 char buffer[16];
								 if (obj.IsTemp())
									 sprintf_s(buffer, "[temp%d]", obj.GetValue());
								 else
									 sprintf_s(buffer, "[%04X]", obj.GetValue());
								 os << buffer;
								 break;
	}
	case TACOperand::ADDRESS:
	{
								  char buffer[16];
								  sprintf_s(buffer, "%04X", obj.GetValue());
								  os << buffer;
								  break;
	}
	case TACOperand::REGISTER:
	{
								   os << Nes::ToString((NesRegisters)obj.GetValue());
								   break;
	}
	}
	return os;
}


OStream& operator<<(OStream& out, const TAC* tuple)
{
	switch (tuple->op)
	{
	case TACOperator::ADD:
		out << tuple->z << " = " << tuple->x << " + " << tuple->y;
		break;
	case TACOperator::SUB:
		out << tuple->z << " = " << tuple->x << " - " << tuple->y;
		break;
	case TACOperator::BOR:
		out << tuple->z << " = " << tuple->x << " | " << tuple->y;
		break;
	case TACOperator::BAND:
		out << tuple->z << " = " << tuple->x << " & " << tuple->y;
		break;
	case TACOperator::BNOT:
		out << tuple->z << " = ~" << tuple->x;
		break;
	case TACOperator::XOR:
		out << tuple->z << " = " << tuple->x << " ^ " << tuple->y;
		break;
	case TACOperator::ASSIGN:
		out << tuple->z << " = " << tuple->x;
		break;
	case TACOperator::INDEX:
		if (tuple->y.IsZero())
			out << tuple->z << " = [" << tuple->x << "]";
		else
			out << tuple->z << " = " << tuple->x << "[" << tuple->y << "]";
		break;
	case TACOperator::REF:
		if (tuple->y.IsZero())
			out << "[" << tuple->x << "] = " << tuple->z;
		else
			out << tuple->x << "[" << tuple->y << "] = " << tuple->z;
		break;
	case TACOperator::IFGREAT:
		out << "if " << tuple->x << " > " << tuple->y << " goto " << tuple->z;
		break;
	case TACOperator::IFGEQ:
		out << "if " << tuple->x << " >= " << tuple->y << " goto " << tuple->z;
		break;
	case TACOperator::IFLESS:
		out << "if " << tuple->x << " < " << tuple->y << " goto " << tuple->z;
		break;
	case TACOperator::IFNEQ:
		out << "if " << tuple->x << " != " << tuple->y << " goto " << tuple->z;
		break;
	case TACOperator::IFEQ:
		out << "if " << tuple->x << " == " << tuple->y << " goto " << tuple->z;
		break;
	case TACOperator::CALL:
		out << "call " << tuple->z;
		break;
	case TACOperator::GOTO:
		out << "goto " << tuple->z;
		break;
	case TACOperator::RETURN:
		out << "return";
		break;
	case TACOperator::SHL:
		out << tuple->z << " = " << tuple->x << " << " << tuple->y;
		break;
	case TACOperator::SHR:
		out << tuple->z << " = " << tuple->x << " >> " << tuple->y;
		break;
	case TACOperator::ROL:
		out << tuple->z << " = " << tuple->x << " O< " << tuple->y;
		break;
	case TACOperator::ROR:
		out << tuple->z << " = " << tuple->x << " O> " << tuple->y;
		break;
	case TACOperator::PUSH:
		out << "push " << tuple->x;
		break;
	case TACOperator::POP:
		out << "pop " << tuple->z;
		break;

	case TACOperator::BREAK:
		out << "BRK";
		break;
	case TACOperator::BIT:
		out << tuple->x << " BIT " << tuple->y;  // 暂时先这样
		break;
	default:
		throw Exception(_T("输出三地址码: 未实现的三地址码操作码"));
	}
	return out;
}

OStream& DumpAddressTAC(OStream& os, const TAC* tac)
{
	TCHAR buffer[32];
	_stprintf_s(buffer, _T("%04X    "), tac->address);
	os << buffer << tac;
	return os;
}

void DumpTACSubroutineAXY(TACSubroutine* sub)
{
	static const TCHAR* axyStr[] =
	{
		_T("无"), _T("A"), _T("X"), _T("AX"), _T("Y"), _T("AY"), _T("XY"), _T("AXY")
	};
	static int count = 0;

	int params = sub->flag & 7;
	int returns = (sub->flag >> 3) & 7;
	TCHAR buffer[128];

	_stprintf_s(buffer, _T("%d 函数 %04X 的分析结果，参数: %s，返回值: %s\n"), count++, sub->GetStartAddress(),
		axyStr[params], axyStr[returns]);
	COUT << buffer;
}

TACOperand::TACOperand(uint32_t value) :
data(value)
{

}

TACOperand::TACOperand() :
data(0)
{

}

bool TACOperand::operator==(const TACOperand& other)
{
	return data == other.data;
}

TACSubroutine::TACSubroutine() :
tempCount(0)
{

}

TACSubroutine::TACSubroutine(Nes::Address startAddress, Nes::Address endAddress):
NesRegion(startAddress, endAddress)
{

}

void TACSubroutine::Dump()
{
	for (auto tac : codes)
	{
		DumpAddressTAC(COUT, tac) << std::endl;
	}
}

TACBasicBlock::TACBasicBlock()
{

}

TACBasicBlock::TACBasicBlock(Nes::Address startAddress, Nes::Address endAddress):
NesRegion(startAddress, endAddress)
{

}
