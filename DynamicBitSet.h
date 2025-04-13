#pragma once
#include <boost/dynamic_bitset/dynamic_bitset.hpp>

// 使用指定类型来表示任意 位 的位集
// 最多可以包含指定类型包含的位数
class DynamicBitSet
{
public:
	DynamicBitSet() : data(0) {}
	DynamicBitSet(size_t size):data(size){}
	DynamicBitSet(const boost::dynamic_bitset<>& value) : data(value) {}
	DynamicBitSet(std::initializer_list<int> values)
	{
		// 使用 std::max_element 查找最大值
		auto max_it = std::max_element(values.begin(), values.end());
		if (max_it == values.end())
			return;

		data.resize(*max_it + 1);
		for (auto v : values)
			*this += v;
	}

	// 获取全集
	static DynamicBitSet FullSet(size_t count)
	{
		DynamicBitSet s(count);
		s.Flip();
		return s;
	}

	// 获取位集中为1的位的索引
	std::vector<int> ToVector() const
	{
		std::vector<int> vec;
		vec.reserve(16);
		for (int i = 0; i < (int)data.size(); ++i)
		{
			if (data[i])
				vec.push_back(i);
		}
		return vec;
	}
	// 获取位集的整数表示
	//inline T ToInteger() const { return data; }
	// 获取位集中1的个数
	int Count() const
	{
		return (int)data.count();
	}
	// 设置大小
	void Resize(size_t size)
	{
		data.resize(size);
	}
	// 并上另一个位集
	inline DynamicBitSet& operator|=(const DynamicBitSet& other) { data |= other.data; return *this; }
	// 获取两个位集的并集
	inline DynamicBitSet operator|(const DynamicBitSet& other) const { return DynamicBitSet(data | other.data); }
	// 与另一个位集相交
	inline DynamicBitSet& operator&=(const DynamicBitSet& other) { data &= other.data; return *this; }
	// 获取两个位集的交集
	inline DynamicBitSet operator&(const DynamicBitSet& other) const { return DynamicBitSet(data & other.data); }
	// 对位集进行取反操作，也就是求补集
	inline DynamicBitSet operator~() const { return DynamicBitSet(~data); }
	// 添加一个元素
	inline DynamicBitSet& operator+=(int index) { data.set(index, true); return *this; }
	// 添加指定位集中的元素
	inline DynamicBitSet& operator+=(const DynamicBitSet& a) { data |= a.data; return *this; }
	// 移除一个元素
	inline DynamicBitSet& operator-=(int index) { data.set(index, false); return *this; }
	// 移除指定位集中的元素
	inline DynamicBitSet& operator-=(const DynamicBitSet& a) { data &= ~a.data; return *this; }
	// 获取移除一个元素后的位集
	inline DynamicBitSet operator-(int index) const
	{
		auto t = data;
		t.set(index, false);
		return DynamicBitSet(t);
	}
	// 给集合赋值
	//inline DynamicBitSet& operator=(T value) { data = value; return *this; }
	// 判断两个位集是否相等
	inline bool operator==(const DynamicBitSet& other) const { return data == other.data; }
	inline bool operator==(const boost::dynamic_bitset<>& value) const { return data == value; }
	// 判断两个位集是否不等
	inline bool operator!=(const DynamicBitSet& other) const { return data != other.data; }
	inline bool operator!=(const boost::dynamic_bitset<>& value) const { return data != value; }
	// 位集中是否包含指定元素
	inline bool Contains(int index) const { return data[index]; }
	// 替换位集中的 a 元素为 b 元素
	inline DynamicBitSet& Replace(int a, int b)
	{
		*this -= a;
		*this += b;
		return *this;
	}
	// 替换位集中的子位集 a 为指定位集 b
	inline DynamicBitSet& Replace(const DynamicBitSet& a, const DynamicBitSet& b)
	{
		data = data & ~a.data | b.data;
		return *this;
	}
	// 是否包含有元素
	inline bool Any() const { return data.any(); }
	// 是否为空
	inline bool None() const { return data.none(); }
	// 取反
	inline DynamicBitSet& Flip()
	{
		data.flip();
		return *this;
	}
private:
	boost::dynamic_bitset<> data;
};
