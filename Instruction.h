#pragma once

struct Instruction
{
	Nes::Address address;  // ָ��ĵ�ַ
	uint32_t data;  // ָ�������

	// ��ȡָ�����Ŀ��Ϣ
	const Nes::OpcodeEntry& GetEntry() const;
	// ��ȡָ��Ĳ�����
	Nes::Opcode GetOpcode() const;
	// ��ȡָ����ֽڳ���
	int GetLength() const;
	// ��ȡ��һ���������ֽ�
	inline int GetByte() const { return (data >> 8) & 0xFF; }
	// ���������������ֽڵ�����ַ����
	inline Nes::Address GetOperandAddress() const
	{
		return (data >> 8) & 0xFFFF;
	}
	// ��ȡ������ת��Ŀ���ַ
	inline Nes::Address GetConditionalJumpAddress() const
	{
		// ������תָ���Ϊ 2
		return (char)GetByte() + address + 2;
	}
	// ����ָ�������
	inline void SetData(const uint8_t* data) { this->data = *(uint32_t*)data; }
	// ����ָ��ĵ�ַ
	inline void SetAddress(Nes::Address addr) { address = addr; }
	// ����ָ������ݺ͵�ַ
	inline void Set(Nes::Address addr, const uint8_t* data_) { address = addr; data = *(uint32_t*)data_; }
	// ��ȡ�������ֽ�
	inline int GetOperatorByte() const { return data & 0xFF; }
	// ��ȡָ��ĵ�ַ
	inline Nes::Address GetAddress() const { return address; }
};

// �򵥵ػ�ȡָ����ַ�����ʾ
const TCHAR* FormatInstruction(const Instruction& instruction);
