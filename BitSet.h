#pragma once

// ���� λ ��λ��
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
	// ��ȡλ����Ϊ1��λ������
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
	// ��ȡλ����������ʾ
	inline T ToInteger() const { return data; }
	// ��ȡλ����1�ĸ���
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
	// ������һ��λ��
	inline BitSet& operator|=(const BitSet other) { data |= other.data; return *this; }
	// ��ȡ����λ���Ĳ���
	inline BitSet operator|(const BitSet other) const { return BitSet(data | other.data); }
	// ����һ��λ���ཻ
	inline BitSet& operator&=(const BitSet other) { data &= other.data; return *this; }
	// ��ȡ����λ���Ľ���
	inline BitSet operator&(const BitSet other) const { return BitSet(data & other.data); }
	// ��λ������ȡ��������Ҳ�����󲹼�
	inline BitSet operator~() const { return BitSet(~data); }
	// ���һ��Ԫ��
	inline BitSet& operator+=(int index) { data |= (T)(1 << index); return *this; }
	// ���ָ��λ���е�Ԫ��
	inline BitSet& operator+=(const BitSet& a) { data |= a.data; return *this; }
	// �Ƴ�һ��Ԫ��
	inline BitSet& operator-=(int index) { data &= ~(1 << index); return *this; }
	// �Ƴ�ָ��λ���е�Ԫ��
	inline BitSet& operator-=(const BitSet& a) { data &= ~a.data; return *this; }
	// ��ȡ�Ƴ�һ��Ԫ�غ��λ��
	inline BitSet operator-(int index) const { return BitSet(data & ~(1 << index)); }
	// �����ϸ�ֵ
	inline BitSet& operator=(T value) { data = value; return *this; }
	// �ж�����λ���Ƿ����
	inline bool operator==(const BitSet other) const { return data == other.data; }
	inline bool operator==(T value) const { return data == value; }
	// �ж�����λ���Ƿ񲻵�
	inline bool operator!=(const BitSet other) const { return data != other.data; }
	inline bool operator!=(T value) const { return data != value; }
	// λ�����Ƿ����ָ��Ԫ��
	inline bool Contains(int index) const { return (data & (T)(1 << index)) != 0; }
	// �滻λ���е� a Ԫ��Ϊ b Ԫ��
	inline BitSet& Replace(int a, int b) { *this -= a; *this += b; return *this; }
	// �滻λ���е���λ�� a Ϊָ��λ�� b
	inline BitSet& Replace(const BitSet& a, const BitSet& b)
	{
		data = data & ~a.data | b.data;
		return *this;
	}
	// �Ƿ������Ԫ��
	inline bool Any() const { return data != 0; }
	// �Ƿ�Ϊ��
	inline bool None() const { return data == 0; }
private:
	T data;
};

// 32 λ ��λ��
using BitSet8 = BitSet<uint8_t>;
using BitSet16 = BitSet<uint16_t>;
using BitSet32 = BitSet<uint32_t>;
using BitSet64 = BitSet<uint64_t>;
