#include "stdafx.h"
#include "StringJoiner.h"



StringJoiner::StringJoiner(size_t capacity /*= 256*/) :
	buffer(capacity)
{
	pFront = &buffer[0];
	pBack = &buffer[0] + buffer.size();
}

StringJoiner::StringJoiner(const StringJoiner& other) :
	buffer(other.buffer)
{
	pFront = &buffer[0] + (other.pFront - &other.buffer[0]);
	pBack = pFront + (other.pBack - other.pFront);
}

StringJoiner::StringJoiner(StringJoiner&& other) :
	buffer(std::move(other.buffer)),
	pFront(other.pFront),
	pBack(other.pBack)
{

}

StringJoiner::~StringJoiner()
{

}

std::basic_string<TCHAR> StringJoiner::ToString() const
{
	size_t size = GetLength();
	std::basic_string<TCHAR> result;
	result.reserve(size + 1);
	result.append(pBack, (TCHAR*)&buffer[0] + buffer.size());
	result.append((TCHAR*)&buffer[0], pFront);
	return result;
}

void StringJoiner::Clear()
{
	pFront = &buffer[0];
	pBack = &buffer[0] + buffer.size();
	buffer[0] = _T('\0');
}

StringJoiner& StringJoiner::operator<<(const TCHAR* str)
{
	if (!str)
		throw std::runtime_error("Argument str is nullptr in PushFront");
	size_t len = _tcslen(str);
	if (pBack - len < pFront)
		Expand(len);
	pBack -= len;
	std::copy_n(str, len, pBack);
	return *this;
}

StringJoiner& StringJoiner::operator>>(const TCHAR* str)
{
	if (!str)
		throw std::runtime_error("Argument str is nullptr in PushBack");
	size_t len = _tcslen(str);
	if (pFront + len >= pBack)
		Expand(len);
	pFront = std::copy_n(str, len, pFront);
	return *this;
}

void StringJoiner::Expand(size_t length)
{
	size_t len = std::max(GetLength() + length, buffer.size() * 2);  // 至少两倍
	len += len / 2;  // 再增加50%的额外空间
	decltype(buffer) newBuffer(len);
	// 复制前向字符串
	pFront = std::copy((TCHAR*)&buffer[0], pFront, &newBuffer[0]);
	// 复制后向字符串
	size_t backLength = &buffer[0] + buffer.size() - pBack;
	TCHAR* pBackNew = &newBuffer[0] + newBuffer.size() - backLength;
	std::copy_n(pBack, backLength, pBackNew);
	pBack = pBackNew;
	buffer = std::move(newBuffer);
}
