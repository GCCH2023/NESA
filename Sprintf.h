#pragma once
#include <cstdarg>
#include <stdexcept>
#include <cstring>

template<size_t N = 256>
class Sprintf
{
public:
	// ���캯��
	Sprintf() : length(0) {
		buffer[0] = '\0'; // ȷ����ʼ״̬Ϊ���ַ���
	}

	// ��������
	~Sprintf() {}

	// ��ʽ���ַ�����������
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

	// �������ַ�����׷�Ӹ�ʽ���ַ���
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
			buffer[N - 1] = '\0'; // ȷ���ַ�����null��β
			length = N - 1;
		}
		return buffer;
	}

	// ��ʽת��Ϊconst TCHAR*���Ա�ʹ�ø�ʽ������ַ���
	operator const TCHAR*() const {
		return buffer;
	}

	inline const TCHAR* ToString() const {
		return buffer;
	}

	// ��ջ�����
	void Clear() {
		buffer[0] = '\0';
		length = 0;
	}

	// ��ȡ��ǰ�ַ�������
	size_t GetLength() const {
		return length;
	}

private:
	TCHAR buffer[N]; // ���������ڴ洢��ʽ������ַ���
	size_t length; // ��ǰ���������ַ����ĳ���
};