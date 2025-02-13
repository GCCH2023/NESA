#pragma once

struct Instruction
{
	Nes::Address address;  // 指令的地址
	uint32_t data;  // 指令的数据

	// 获取指令的条目信息
	const Nes::OpcodeEntry& GetEntry() const;
	// 获取指令的操作码
	Nes::Opcode GetOpcode() const;
	// 获取指令的字节长度
	int GetLength() const;
	// 获取第一个操作数字节
	inline int GetByte() const { return (data >> 8) & 0xFF; }
	// 将操作数的两个字节当作地址返回
	inline Nes::Address GetOperandAddress() const
	{
		return (data >> 8) & 0xFFFF;
	}
	// 获取条件跳转的目标地址
	inline Nes::Address GetConditionalJumpAddress() const
	{
		// 条件跳转指令长度为 2
		return (char)GetByte() + address + 2;
	}
	// 设置指令的数据
	inline void SetData(const uint8_t* data) { this->data = *(uint32_t*)data; }
	// 设置指令的地址
	inline void SetAddress(Nes::Address addr) { address = addr; }
	// 设置指令的数据和地址
	inline void Set(Nes::Address addr, const uint8_t* data_) { address = addr; data = *(uint32_t*)data_; }
	// 获取操作码字节
	inline int GetOperatorByte() const { return data & 0xFF; }
	// 获取指令的地址
	inline Nes::Address GetAddress() const { return address; }
};

// 简单地获取指令的字符串表示
const TCHAR* FormatInstruction(const Instruction& instruction);
