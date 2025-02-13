#include "stdafx.h"
#include "Mappers.h"
#include "Cartridge.h"

CMapper000::CMapper000(Cartridge& cartridge) :
CMapper(cartridge)
{

}

uint8_t CMapper000::ReadByte(Nes::Address virtualAddress)
{
	int offset = ToFileOffset(virtualAddress);
	return cartridge.RawReadByte(offset);
}

uint16_t CMapper000::ReadWord(Nes::Address virtualAddress)
{
	int offset = ToFileOffset(virtualAddress);
	return cartridge.RawReadWord(offset);
}


int CMapper000::ToFileOffset(Nes::Address virtualAddress) const
{
	if (virtualAddress < 0x8000 || virtualAddress >= 0x10000)
	{
		char buffer[128];
		sprintf_s(buffer, "虚拟地址 %04X 超出范围", virtualAddress);
		throw buffer;
	}
	if (cartridge.GetPRGCount() > 1)
		return cartridge.GetPRGOffset() + virtualAddress - 0x8000;
	else
		return cartridge.GetPRGOffset() + ((virtualAddress - 0x8000) & 0x3FFF);
}

std::shared_ptr<CMapper> GetMapper(Cartridge& cartridge)
{
	switch (cartridge.GetMapperNumber())
	{
	case 0: return std::make_shared<CMapper000>(cartridge);
	}
	throw Exception(_T("未实现的 mapper!"));
}
