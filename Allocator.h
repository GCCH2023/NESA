#pragma once

class Allocator
{
public:
	Allocator(uint32_t capacity_ = 32 * 1024 * 1024) :
		capacity(capacity_)
	{
		baseAddress = (uint8_t*)VirtualAlloc(nullptr, capacity, MEM_RESERVE, PAGE_READWRITE);
		current = endAddress = baseAddress;
	}

	~Allocator()
	{
		if (baseAddress)
			VirtualFree(baseAddress, capacity, MEM_RELEASE);
	}

	// ����ָ����С���ڴ�
	template<typename T>
	T* Alloc()
	{
		T* obj = (T *)current;
		current += sizeof(T);
		if (current > endAddress)
			Expand();
		return obj;
	}

	// ����Ԫ������
	template<typename T>
	T* Alloc(size_t count)
	{
		T* obj = (T *)current;
		current += sizeof(T)* count;
		if (current > endAddress)
			Expand();
		return obj;
	}

	// ����һ�����󲢵������Ĺ��캯��
	template<typename T, typename... Args>
	inline T* New(Args&&... args) {
		T* obj = Alloc<T>();
		obj = new(obj)T(std::forward<Args>(args)...);
		return obj;
	}

	// �����ڴ�
	void Reset()
	{
		memset(baseAddress, 0, current - baseAddress);
		current = baseAddress;
	}
private:
	// �ύ�µ��ڴ��
	void Expand()
	{
		if ((uint32_t)(current - baseAddress) > capacity)
			throw std::bad_alloc();

		uint32_t mask = 16 * 1024 - 1;
		uint32_t size = 16 * 1024;
		size = (current - endAddress + mask) & ~mask;
		if (!VirtualAlloc(endAddress, size, MEM_COMMIT, PAGE_READWRITE))
			throw std::bad_alloc();
		endAddress += size;
	}
private:
	uint8_t* baseAddress;  // �ڴ����ַ
	uint8_t* current;  // ��ǰ����λ��
	uint8_t* endAddress;  // ���ύ�ڴ��ĩβλ��
	uint32_t capacity;  // �������ڴ������
};