#pragma once
#include "Mapper.h"

// 0 号映射器
// 程序部分只可能是32kb或16kb，图像部分是8kb
// 如果程序部分是32kb，则映射到 0x8000 - 0x10000
// 如果程序部分是16kb，则映射到 0x8000 - 0xA000, 并镜像 0xA000 - 0x10000
// 图像部分映射到 PPU 0x0 - 0x2000
class CMapper000 : public CMapper
{
public:
	CMapper000(Cartridge& cartridge);
	// 读取虚拟地址处的一个字节
	virtual uint8_t ReadByte(Nes::Address virtualAddress) override;
	virtual uint16_t ReadWord(Nes::Address virtualAddress) override;
	virtual int ToFileOffset(Nes::Address virtualAddress) const override;
};

// 获取卡带对应的映射器
std::shared_ptr<CMapper> GetMapper(Cartridge& cartridge);
