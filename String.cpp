#include "stdafx.h"
#include "String.h"


uint32_t GetStringHash(const TCHAR* str, size_t* pLen)
{
	uint32_t hash = 0;
	size_t len;
	for (len = 0; str[len]; ++len)
		hash = (uint8_t)str[len] + (hash >> 4) + 4 * hash;
	if (pLen)
		*pLen = len;
	return hash;
}


StringTable::StringTable(Allocator& allocator_) :
allocator(allocator_)
{
	table = allocator.Alloc<String*>(STR_TABLE_CAPACITY);
}

String* StringTable::AddString(const TCHAR* str)
{
	if (!str)
		return nullptr;

	size_t len;
	uint32_t hash = GetStringHash(str, &len);
	int index = (hash & STR_TABLE_CAPACITY);
	String** pHead = &table[index];
	hash = (uint16_t)hash ^ HIWORD(hash);
	String** prev = pHead;
	String* s;
	for (s = *pHead; s; s = s->next)
	{
		if ((uint16_t)s->hash == hash && !s->str[len])  // 哈希值相等且长度相等
		{
			if (!memcmp(str, s->str, len))
			{
				*prev = s->next;
				s->next = *pHead;
				*pHead = s;
				return s;
			}
		}
		prev = &s->next;
	}
	s = allocator.New<String>();
	s->next = *pHead;
	*pHead = s;
	size_t size = (len + 4) & ~3;
	TCHAR* strData = allocator.Alloc<TCHAR>(size);
	s->str = strData;
	strcpy_s(strData, size, str);
	s->hash = hash;
	return s;
}

String* StringTable::GetString(const TCHAR* str)
{
	if (!str)
		return nullptr;

	size_t len;
	uint32_t hash = GetStringHash(str, &len);
	String** pHead = &table[hash & STR_TABLE_CAPACITY];
	hash = (uint16_t)hash ^ HIWORD(hash); // 哈希值
	String* s;
	for (s = *pHead; s; s = s->next)
	{
		if ((uint16_t)s->hash == hash && !s->str[len])
		{
			if (!memcmp(str, s->str, len))
			{
				return s;
			}
		}
	}
	return nullptr;
}