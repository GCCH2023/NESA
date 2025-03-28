#pragma once

// 内存分配器
class Allocator
{
public:
	Allocator(uint32_t capacity_ = 32 * 1024 * 1024) :
		capacity(capacity_)
	{
		baseAddress = (uint8_t*)VirtualAlloc(nullptr, capacity, MEM_RESERVE, PAGE_READWRITE);
		if (!baseAddress)
			throw std::bad_alloc();
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
		size_t align = __alignof(T);
		current = (uint8_t*)(((size_t)current + align - 1) & ~(align - 1));
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
		size_t align = __alignof(T);
		current = (uint8_t*)(((size_t)current + align - 1) & ~(align - 1));
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
	// clear: 是否清零内存
	void Reset(bool clear = 0)
	{
		memset(baseAddress, 0, current - baseAddress);
		current = baseAddress;
	}

	// 获取当前分配点
	size_t Mark() const
	{
		return static_cast<size_t>(current - baseAddress);
	}

	// 回滚到指定分配点（释放后续分配的内存）
	void Rollback(size_t mark) {
		if (mark > static_cast<size_t>(endAddress - baseAddress))
		{
			throw std::out_of_range("Invalid mark: beyond committed memory");
		}
		current = baseAddress + mark;
	}
private:
	// 提交新的内存块
	void Expand()
	{
		if ((size_t)(current - baseAddress) > capacity)
			throw std::bad_alloc();

		size_t mask = 16 * 1024 - 1;
		size_t size = 16 * 1024;
		size = (current - endAddress + mask) & ~mask;
		if (!VirtualAlloc(endAddress, size, MEM_COMMIT, PAGE_READWRITE))
			throw std::bad_alloc();
		endAddress += size;
	}
private:
	uint8_t* baseAddress;  // 内存基地址
	uint8_t* current;  // 当前分配位置
	uint8_t* endAddress;  // 已提交内存的末尾位置
	size_t capacity;  // 保留的内存的容量
};