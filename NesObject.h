#pragma once

// NES 中的一个地址区间
// 结束地址不包括在区间内
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
	Nes::Address startAddress;  // 开始地址
	Nes::Address endAddress;  // 结束地址
public:
	void* tag;  // 扩展使用，在执行完一些算法后会设置它的值
};