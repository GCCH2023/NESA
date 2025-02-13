#include "stdafx.h"
#include "Exception.h"


Exception::Exception()
{
}


Exception::Exception(const TCHAR* message_) :
message(message_)
{

}

Exception::~Exception()
{
}

const TCHAR* Exception::Message() const
{
	return message.c_str();
}
