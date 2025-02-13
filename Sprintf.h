#pragma once
#include <cstdarg>
#include <stdexcept>
#include <cstring>

template<size_t N = 256>
class Sprintf
{
public:
	// 构造函数
	Sprintf() : length(0) {
		buffer[0] = '\0'; // 确保初始状态为空字符串
	}

	// 析构函数
	~Sprintf() {}

	// 格式化字符串到缓冲区
	const TCHAR* Format(const TCHAR* format, ...) {
		va_list args;
		va_start(args, format);
		int result = _vstprintf_s(buffer, N, format, args);
		va_end(args);

		if (result < 0) {
			throw std::runtime_error("Format failed");
		}
		length = static_cast<size_t>(result);
		return buffer;
	}

	// 在现有字符串后追加格式化字符串
	const TCHAR* Append(const TCHAR* format, ...) {
		if (length >= N) {
			throw std::runtime_error("Buffer overflow in Append");
		}

		va_list args;
		va_start(args, format);
		int result = _vstprintf_s(buffer + length, N - length, format, args);
		va_end(args);

		if (result < 0) {
			throw std::runtime_error("Append failed");
		}
		length += static_cast<size_t>(result);
		if (length >= N) {
			buffer[N - 1] = '\0'; // 确保字符串以null结尾
			length = N - 1;
		}
		return buffer;
	}

	// 隐式转换为const TCHAR*，以便使用格式化后的字符串
	operator const TCHAR*() const {
		return buffer;
	}

	inline const TCHAR* ToString() const {
		return buffer;
	}

	// 清空缓冲区
	void Clear() {
		buffer[0] = '\0';
		length = 0;
	}

	// 获取当前字符串长度
	size_t GetLength() const {
		return length;
	}

private:
	TCHAR buffer[N]; // 缓冲区用于存储格式化后的字符串
	size_t length; // 当前缓冲区中字符串的长度
};