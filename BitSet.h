#pragma once

// 任意 位 的位集
template<typename T>
class BitSet
{
public:
	BitSet() : data(0) {}
	BitSet(T value) : data(value) {}
	BitSet(std::initializer_list<int> values) :
		data(0)
	{
		for (auto v : values)
			*this += v;
	}
	// 获取位集中为1的位的索引
	std::vector<int> ToVector() const
	{
		T s = data;
		std::vector<int> vec(16);
		vec.clear();
		for (int i = 0; s; ++i)
		{
			if (s & 1)
			{
				vec.push_back(i);
			}
			s >>= 1;
		}
		return vec;
	}
	// 获取位集的整数表示
	inline T ToInteger() const { return data; }
	// 获取位集中1的个数
	int GetSize() const
	{
		T s = data;
		int n = 0;
		while (s)
		{
			if (s & 1)
				++n;
			s >>= 1;
		}
		return n;
	}
	// 并上另一个位集
	inline BitSet& operator|=(const BitSet other) { data |= other.data; return *this; }
	// 获取两个位集的并集
	inline BitSet operator|(const BitSet other) const { return BitSet(data | other.data); }
	// 与另一个位集相交
	inline BitSet& operator&=(const BitSet other) { data &= other.data; return *this; }
	// 获取两个位集的交集
	inline BitSet operator&(const BitSet other) const { return BitSet(data & other.data); }
	// 对位集进行取反操作，也就是求补集
	inline BitSet operator~() const { return BitSet(~data); }
	// 添加一个元素
	inline BitSet& operator+=(int index) { data |= (T)(1 << index); return *this; }
	// 添加指定位集中的元素
	inline BitSet& operator+=(const BitSet& a) { data |= a.data; return *this; }
	// 移除一个元素
	inline BitSet& operator-=(int index) { data &= ~(1 << index); return *this; }
	// 移除指定位集中的元素
	inline BitSet& operator-=(const BitSet& a) { data &= ~a.data; return *this; }
	// 获取移除一个元素后的位集
	inline BitSet operator-(int index) const { return BitSet(data & ~(1 << index)); }
	// 给集合赋值
	inline BitSet& operator=(T value) { data = value; return *this; }
	// 判断两个位集是否相等
	inline bool operator==(const BitSet other) const { return data == other.data; }
	inline bool operator==(T value) const { return data == value; }
	// 判断两个位集是否不等
	inline bool operator!=(const BitSet other) const { return data != other.data; }
	inline bool operator!=(T value) const { return data != value; }
	// 位集中是否包含指定元素
	inline bool Contains(int index) const { return (data & (T)(1 << index)) != 0; }
	// 替换位集中的 a 元素为 b 元素
	inline BitSet& Replace(int a, int b) { *this -= a; *this += b; return *this; }
	// 替换位集中的子位集 a 为指定位集 b
	inline BitSet& Replace(const BitSet& a, const BitSet& b)
	{
		data = data & ~a.data | b.data;
		return *this;
	}
	// 是否包含有元素
	inline bool Any() const { return data != 0; }
	// 是否为空
	inline bool None() const { return data == 0; }
private:
	T data;
};

// 32 位 的位集
using BitSet8 = BitSet<uint8_t>;
using BitSet16 = BitSet<uint16_t>;
using BitSet32 = BitSet<uint32_t>;
using BitSet64 = BitSet<uint64_t>;
