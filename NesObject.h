#pragma once

// NES �е�һ����ַ����
// ������ַ��������������
class NesRegion
{
public:
	NesRegion();
	NesRegion(Nes::Address startAddr, Nes::Address endAddr);

	inline Nes::Address GetStartAddress() const { return startAddress; }
	inline void SetStartAddress(Nes::Address addr) { startAddress = addr; }
	inline Nes::Address GetEndAddress() const { return endAddress; }
	inline void SetEndAddress(Nes::Address addr) { endAddress = addr; }
private:
	Nes::Address startAddress;  // ��ʼ��ַ
	Nes::Address endAddress;  // ������ַ
public:
	void* tag;  // ��չʹ�ã���ִ����һЩ�㷨�����������ֵ
};