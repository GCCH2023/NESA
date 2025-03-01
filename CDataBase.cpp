#include "stdafx.h"
#include "CDataBase.h"

CDataBase::CDataBase(Allocator& allocator_) :
allocator(allocator_),
stringTable(allocator_),
typeManager(allocator_)
{

}

CDataBase::~CDataBase()
{
}
