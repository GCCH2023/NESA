#include "stdafx.h"
#include "NesObject.h"

NesRegion::NesRegion():
startAddress(0),
endAddress(0)
{

}

NesRegion::NesRegion(Nes::Address startAddr, Nes::Address endAddr) :
startAddress(startAddr),
endAddress(endAddr)
{

}
