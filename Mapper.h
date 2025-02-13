#pragma once

class Cartridge;
// 映射器
class CMapper
{
public:
	CMapper(Cartridge& cartridge);
	// 从指定虚拟地址处读取1个字节
	virtual uint8_t ReadByte(Nes::Address virtualAddress) = 0;
	// 从指定虚拟地址处读取2个字节
	virtual uint16_t ReadWord(Nes::Address virtualAddress) = 0;
	// 将指定虚拟地址转换为 ROM 文件偏移量
	// !!! 同一个虚拟地址可能在不同时刻对应不同的 文件偏移量
	virtual int ToFileOffset(Nes::Address virtualAddress) const = 0;
	const uint8_t* GetData(Nes::Address address) const;
protected:
	Cartridge& cartridge;
};
