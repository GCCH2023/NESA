#pragma once

// 字符串
struct String
{
	String* next;  // 下一个字符串
	const TCHAR* str;  // 字符串数据
	uint32_t hash;  // 哈希值
};

// 计算字符串对应的哈希值
uint32_t GetStringHash(const TCHAR* str, size_t* pLen);

#define STR_TABLE_CAPACITY 0x7FF

// 字符串表
class StringTable
{
public:
	StringTable(Allocator& allocator);

	// 添加指定字符串到字符串表中，如果存在，返回已有的，否则创建
	String* AddString(const TCHAR* str);
	// 获取指定字符串对象，不存在则返回空
	String* GetString(const TCHAR* str);

protected:
	Allocator& allocator;
	String** table;  // 字符串哈希表
};