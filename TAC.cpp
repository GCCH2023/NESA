#include "stdafx.h"
#include "TAC.h"
using namespace Nes;

TACOperand RegisterP(TACOperand::REGISTER | TAC_REG_P);
TACOperand RegisterA(TACOperand::REGISTER | TAC_REG_A);
TACOperand RegisterX(TACOperand::REGISTER | TAC_REG_X);
TACOperand RegisterY(TACOperand::REGISTER | TAC_REG_Y);
TACOperand RegisterSP(TACOperand::REGISTER | TAC_REG_SP);
TACOperand RegisterN(TACOperand::REGISTER | TAC_REG_N);
TACOperand RegisterV(TACOperand::REGISTER | TAC_REG_V);
TACOperand RegisterZ(TACOperand::REGISTER | TAC_REG_Z);
TACOperand RegisterC(TACOperand::REGISTER | TAC_REG_C);


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

TAC::TAC(TACOperator op_, TACOperand z_) :
op(op_),
z(z_)
{

}

TAC::TAC(const TAC* tac):
op(tac->op),
x(tac->x),
y(tac->y),
z(tac->z)
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
		_T("IFEQ"),
		_T("IFNEQ"),
		_T("IFTRUE"),
		_T("IFFALSE"),
		_T("ARRAY_GET"),
		_T("ARRAY_SET"),
		_T("ADDR"),
		_T("DEREF"),
		_T("CAST"),
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
		_T("CLI"),
		_T("SEI"),
		_T("CLC"),
		_T("SEC"),
		_T("CLV"),
		_T("CLD"),
		_T("SED"),

		_T("FLAGN"),

		_T("BOOL_GREAT"),
		_T("BOOL_GEQ"),
		_T("BOOL_LESS"),
		_T("BOOL_LEQ"),
		_T("BOOL_EQ"),
		_T("BOOL_NEQ"),
		_T("BOOL_BIT"),
	};
	return names[(int)op];
}

const TCHAR* ToString(TACRegister reg)
{
	switch (reg)
	{
	case TAC_REG_A: return _T("A");
	case TAC_REG_X: return _T("X");
	case TAC_REG_Y: return _T("Y");
	case TAC_REG_N: return _T("N");
	case TAC_REG_V: return _T("V");
	case TAC_REG_Z: return _T("Z");
	case TAC_REG_C: return _T("C");
	case TAC_REG_P: return _T("P");
	case TAC_REG_SP: return _T("SP");
	}
	return nullptr;
}

template <typename T>
T RotateLeft(T value, int shift)
{
	const int bits = sizeof(T)* CHAR_BIT; // 计算类型的位数
	shift %= bits; // 确保 shift 在有效范围内
	return (value << shift) | (value >> (bits - shift));
}

template <typename T>
T RotateRight(T value, int shift)
{
	const int bits = sizeof(T)* CHAR_BIT; // 计算类型的位数
	shift %= bits; // 确保 shift 在有效范围内
	return (value >> shift) | (value << (bits - shift));
}


TACOperand Evaluate(TACOperator op, TACOperand x, TACOperand y)
{
	if (!x.IsInterger() || !y.IsInterger())
		throw Exception(_T("要求值的两个操作数不是整数"));
	// 因为是 NES 相关的三地址码，所以只考虑一个字节的运算
	uint8_t a = x.GetValue();
	uint8_t b = y.GetValue();
	switch (op)
	{
	case TACOperator::ADD: return TACOperand((uint8_t)(a + b));
	case TACOperator::SUB:  return TACOperand((uint8_t)(a - b));
	case TACOperator::BOR:  return TACOperand((uint8_t)(a | b));
	case TACOperator::BAND: return TACOperand((uint8_t)(a & b));
	case TACOperator::BNOT:  return TACOperand((uint8_t)~a);
	case TACOperator::XOR: return TACOperand((uint8_t)(a ^ b));
	case TACOperator::SHR: return TACOperand((uint8_t)(a >> b));
	case TACOperator::SHL: return TACOperand((uint8_t)(a << b));
	case TACOperator::ROR: return TACOperand(RotateRight(a, b));
	case TACOperator::ROL: return TACOperand(RotateLeft(a, b));
	case TACOperator::BOOL_EQ: return TACOperand(a == b);
	case TACOperator::BOOL_NEQ: return TACOperand(a != b);
	case TACOperator::BOOL_LESS: return TACOperand(a < b);
	case TACOperator::BOOL_LEQ: return TACOperand(a <= b);
	case TACOperator::BOOL_GREAT: return TACOperand(a > b);
	case TACOperator::BOOL_GEQ: return TACOperand(a >= b);
	}
	Sprintf<> s;
	s.Format(_T("无法进行求值的操作码 "), ToString(op));
	throw Exception(s);
}

OStream& operator<<(OStream& os, const TACOperand& obj)
{
	switch (obj.GetKind())
	{
	case TACOperand::INTEGER:
	{
								char buffer[16];
								sprintf_s(buffer, "0x%02X", obj.GetValue());
								os << buffer;
								break;
	}
	case TACOperand::GLOBAL:
	{
							   char buffer[16];
							   sprintf_s(buffer, "g_%04X", obj.GetValue());
							   os << buffer;
							   break;
	}
	case TACOperand::TEMP:
	{
							 char buffer[16];
							 sprintf_s(buffer, "temp%d", obj.GetValue());
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
								 os << ToString((TACRegister)obj.GetValue());
								 break;
	}
	}
	return os;
}


OStream& operator<<(OStream& out, const TAC* tac)
{
	switch (tac->op)
	{
	case TACOperator::ADD:
		out << tac->z << _T(" = ") << tac->x << _T(" + ") << tac->y;
		break;
	case TACOperator::SUB:
		out << tac->z << _T(" = ") << tac->x << _T(" - ") << tac->y;
		break;
	case TACOperator::BOR:
		out << tac->z << _T(" = ") << tac->x << _T(" | ") << tac->y;
		break;
	case TACOperator::BAND:
		out << tac->z << _T(" = ") << tac->x << _T(" & ") << tac->y;
		break;
	case TACOperator::BNOT:
		out << tac->z << _T(" = ~") << tac->x;
		break;
	case TACOperator::XOR:
		out << tac->z << _T(" = ") << tac->x << _T(" ^ ") << tac->y;
		break;
	case TACOperator::ASSIGN:
		out << tac->z << _T(" = ") << tac->x;
		break;
	case TACOperator::ARRAY_GET:
		if (tac->y.IsZero())
			out << tac->z << _T(" = [") << tac->x << _T("]");
		else
			out << tac->z << _T(" = ") << tac->x << _T("[") << tac->y << _T("]");
		break;
	case TACOperator::ARRAY_SET:
		if (tac->y.IsZero())
			out << _T("[") << tac->x << _T("] = ") << tac->z;
		else
			out << tac->x << _T("[") << tac->y << _T("] = ") << tac->z;
		break;
	case TACOperator::ADDR:
		out << tac->z << _T(" = &") << tac->x;
		break;
	case TACOperator::DEREF:
		out << tac->z << _T(" = *") << tac->x;
		break;
	case TACOperator::CAST:
		out << tac->z << _T(" = (T)") << tac->x;
		break;
	case TACOperator::IFGREAT:
		out << _T("if ") << tac->x << _T(" > ") << tac->y << _T(" goto ") << tac->z;
		break;
	case TACOperator::IFGEQ:
		out << _T("if ") << tac->x << _T(" >= ") << tac->y << _T(" goto ") << tac->z;
		break;
	case TACOperator::IFLESS:
		out << _T("if ") << tac->x << _T(" < ") << tac->y << _T(" goto ") << tac->z;
		break;
	case TACOperator::IFEQ:
		out << _T("if ") << tac->x << _T(" == ") << tac->y << _T(" goto ") << tac->z;
		break;
	case TACOperator::IFNEQ:
		out << _T("if ") << tac->x << _T(" != ") << tac->y << _T(" goto ") << tac->z;
		break;
	case TACOperator::IFTRUE:
		out << _T("if ") << tac->x << _T(" == true goto ") << tac->z;
		break;
	case TACOperator::IFFALSE:
		out << _T("if ") << tac->x << _T(" == false goto ") << tac->z;
		break;
	case TACOperator::CALL:
		out << _T("sub_") << tac->x << _T("(");
		if (tac->y.GetValue())
			out << tac->y.GetValue();
		out << _T(")");
		break;
	case TACOperator::GOTO:
		out << _T("goto ") << tac->z;
		break;
	case TACOperator::RETURN:
		out << _T("return");
		break;
	case TACOperator::SHL:
		out << tac->z << _T(" = ") << tac->x << _T(" << ") << tac->y;
		break;
	case TACOperator::SHR:
		out << tac->z << _T(" = ") << tac->x << _T(" >> ") << tac->y;
		break;
	case TACOperator::ROL:
		out << tac->z << _T(" = ") << tac->x << _T(" O< ") << tac->y;
		break;
	case TACOperator::ROR:
		out << tac->z << _T(" = ") << tac->x << _T(" O> ") << tac->y;
		break;
	case TACOperator::PUSH:
		out << _T("push ") << tac->x;
		break;
	case TACOperator::POP:
		out << _T("pop ") << tac->z;
		break;

	case TACOperator::BREAK:
		out << _T("BRK");
		break;
	case TACOperator::BIT:
		out << tac->z << _T(" = ") << tac->x << _T(" BIT ") << tac->y;  // 暂时先这样
		break;
	case TACOperator::ARG:
		out << _T("ARG ") << tac->x;  // 暂时先这样
		break;
	case TACOperator::CLI:
	case TACOperator::SEI:
	case TACOperator::CLD:
	case TACOperator::SED:
	case TACOperator::CLC:
	case TACOperator::SEC:
	case TACOperator::CLV:
		out << ToString(tac->op);
		break;
	case TACOperator::BOOL_FLAGV:
		out << tac->z << _T(" = ") << tac->x << _T(" FLAGV ") << tac->y;
		break;
	case TACOperator::BOOL_GREAT:
		out << tac->z << _T(" = ") << tac->x << _T(" > ") << tac->y;
		break;
	case TACOperator::BOOL_GEQ:
		out << tac->z << _T(" = ") << tac->x << _T(" >= ") << tac->y;
		break;
	case TACOperator::BOOL_LESS:
		out << tac->z << _T(" = ") << tac->x << _T(" < ") << tac->y;
		break;
	case TACOperator::BOOL_LEQ:
		out << tac->z << _T(" = ") << tac->x << _T(" <= ") << tac->y;
		break;
	case TACOperator::BOOL_EQ:
		out << tac->z << _T(" = ") << tac->x << _T(" == ") << tac->y;
		break;
	case TACOperator::BOOL_NEQ:
		out << tac->z << _T(" = ") << tac->x << _T(" != ") << tac->y;
		break;
	case TACOperator::BOOL_BAND:
		out << tac->z << _T(" = (") << tac->x << _T(" & ") << tac->y << _T(") != 0");
		break;
	case TACOperator::BOOL_BIT:
		out << tac->z << _T(" = (") << tac->x << _T(" >> ") << tac->y << _T(") & 1");
		break;
	default:
	{
			   Sprintf<> s;
			   s.Format(_T("输出三地址码: 未实现的三地址码操作码 %s"), ToString(tac->op));
			   throw Exception(s.ToString());
	}
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

TACOperator GetNotOperator(TACOperator op)
{
	switch (op)
	{
	case TACOperator::IFTRUE:return TACOperator::IFFALSE;
	case TACOperator::IFFALSE:return TACOperator::IFTRUE;
	case TACOperator::IFGREAT:return TACOperator::IFLEQ;
	case TACOperator::IFGEQ:return TACOperator::IFLESS;
	case TACOperator::IFLESS:return TACOperator::IFGEQ;
	case TACOperator::IFLEQ:return TACOperator::IFGREAT;
	case TACOperator::IFEQ:return TACOperator::IFNEQ;
	case TACOperator::IFNEQ:return TACOperator::IFEQ;
	default:
	{
			   Sprintf<> s;
			   s.Format(_T("三地址码操作符取反: 未实现的操作符 %s"), ToString(op));
			   throw Exception(s.ToString());
	}
	}
}

TACOperand::TACOperand(uint32_t value) :
data(value)
{

}

TACOperand::TACOperand() :
data(0)
{

}

