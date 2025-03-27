#pragma once
#include <stddef.h>

// 可以往前后添加字符串的缓冲区类
class StringJoiner
{
public:
	StringJoiner(size_t capacity = 256);
	StringJoiner(const StringJoiner& other);
	StringJoiner(StringJoiner&& other);
	virtual ~StringJoiner();

	// 获取字符串表示
	std::basic_string<TCHAR> ToString() const;

	// 清空缓冲区
	void Clear();

	// 添加一个字符串到当前字符串的前面
	StringJoiner& operator>>(const TCHAR* str);

	// 添加一个字符串到当前字符串的后面
	StringJoiner& operator<<(const TCHAR* str);

	// 获取当前字符串的长度，不包括 '\0'
	inline size_t GetLength() const
	{
		return pFront + buffer.size() - pBack;
	}

protected:
	// 扩大缓冲区
	// length: 需要的字符串长度
	void Expand(size_t length);

private:
	// 分为两部分，从前往后的字符串，从后往前的字符串
	std::vector<TCHAR> buffer;
	TCHAR* pFront;  // 指向前向字符串的末尾
	TCHAR* pBack;  // 指向后向字符串开头
};