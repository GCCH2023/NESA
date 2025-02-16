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

	// 分配指定大小的内存
	template<typename T>
	T* Alloc()
	{
		T* obj = (T *)current;
		current += sizeof(T);
		if (current > endAddress)
			Expand();
		return obj;
	}

	// 分配元素数组
	template<typename T>
	T* Alloc(size_t count)
	{
		T* obj = (T *)current;
		current += sizeof(T)* count;
		if (current > endAddress)
			Expand();
		return obj;
	}

	// 分配一个对象并调用它的构造函数
	template<typename T, typename... Args>
	inline T* New(Args&&... args) {
		T* obj = Alloc<T>();
		obj = new(obj)T(std::forward<Args>(args)...);
		return obj;
	}

	// 重置内存
	void Reset()
	{
		memset(baseAddress, 0, current - baseAddress);
		current = baseAddress;
	}
private:
	// 提交新的内存块
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
	uint8_t* baseAddress;  // 内存基地址
	uint8_t* current;  // 当前分配位置
	uint8_t* endAddress;  // 已提交内存的末尾位置
	uint32_t capacity;  // 保留的内存的容量
};