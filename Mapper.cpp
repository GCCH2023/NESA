#include "stdafx.h"
#include "Mapper.h"
#include "Cartridge.h"

CMapper::CMapper(Cartridge& cartridge_) :
cartridge(cartridge_)
{
}

const uint8_t* CMapper::GetData(Nes::Address address) const
{
	int offset = ToFileOffset(address);
	return cartridge.RawGetData(offset);
}
