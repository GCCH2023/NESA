#pragma once

// 32 位 的位集
class BitSet32
{
public:
	BitSet32();
	BitSet32(uint32_t value);
	BitSet32(std::initializer_list<int> values);
	// 获取位集中为1的位的索引
	std::vector<int> ToVector() const;
	// 获取位集的整数表示
	inline uint32_t ToInteger() const { return data; }
	// 获取位集中1的个数
	int GetSize() const;
	// 并上另一个位集
	inline BitSet32& operator|=(const BitSet32 other) { data |= other.data; return *this; }
	// 获取两个位集的并集
	inline BitSet32 operator|(const BitSet32 other) const { return BitSet32(data | other.data); }
	// 与另一个位集相交
	inline BitSet32& operator&=(const BitSet32 other) { data &= other.data; return *this; }
	// 获取两个位集的交集
	inline BitSet32 operator&(const BitSet32 other) const { return BitSet32(data & other.data); }
	// 对位集进行取反操作，也就是求补集
	inline BitSet32 operator~() const { return BitSet32(~data); }
	// 添加一个元素
	inline BitSet32& operator+=(int index) { data |= 1 << index; return *this; }
	// 添加指定位集中的元素
	inline BitSet32& operator+=(const BitSet32& a) { data |= a.data; return *this; }
	// 移除一个元素
	inline BitSet32& operator-=(int index) { data &= ~(1 << index); return *this; }
	// 移除指定位集中的元素
	inline BitSet32& operator-=(const BitSet32& a) { data &= ~a.data; return *this; }
	// 获取移除一个元素后的位集
	inline BitSet32 operator-(int index) const { return BitSet32(data & ~(1 << index)); }
	// 给集合赋值
	inline BitSet32& operator=(uint32_t value) { data = value; return *this; }
	// 判断两个位集是否相等
	inline bool operator==(const BitSet32 other) { return data == other.data; }
	inline bool operator==(uint32_t value) { return data == value; }
	// 判断两个位集是否不等
	inline bool operator!=(const BitSet32 other) { return data != other.data; }
	inline bool operator!=(uint32_t value) { return data != value; }
	// 位集中是否包含指定元素
	inline bool Contains(int index) const { return (data & (1 << index)) != 0; }
	// 替换位集中的 a 元素为 b 元素
	inline BitSet32& Replace(int a, int b) { *this -= a; *this += b; return *this; }
	// 替换位集中的子位集 a 为指定位集 b
	inline BitSet32& Replace(const BitSet32& a, const BitSet32& b)
	{
		data = data & ~a.data | b.data;
		return *this;
	}
	// 是否包含有元素
	inline bool Any() const { return data != 0; }
	// 是否为空
	inline bool None() const { return data == 0; }
	// 输出字符串表示
	void Dump() const;
private:
	uint32_t data;
};

using Node = int;
using NodeSet = BitSet32;

// 获取位集对应的节点列表
inline std::vector<Node> Nodes(NodeSet& s)
{
	return s.ToVector();
}

struct Edge
{
	Node from;
	Node to;
};


struct BasicBlock
{
	int index;
	BitSet32 pred;  // 前驱节点集
	BitSet32 succ;  // 后继节点集
	BitSet32 domin;  // 必经节点集

	// 获取后继数量
	inline int GetSuccCount() const { return succ.GetSize(); }
	// 获取前驱数量
	inline int GetPredCount() const { return pred.GetSize(); }
	// 获取所有前驱基本块的索引列表
	inline std::vector<Node> Pred() const { return pred.ToVector(); }
	// 获取所有后继基本块的索引列表
	inline std::vector<Node> Succ() const { return succ.ToVector(); }
	// 获取指定节点的相邻（前驱+后继）节点集合
	inline std::vector<Node> Adjacent()
	{
		BitSet32 total = pred | succ;
		return total.ToVector();
	}
	// 输出字符串表示
	void Dump();
};

enum CtrlTreeNodeType
{
	CTNTYPE_LEAF,  // 叶子区域
	CTNTYPE_LIST,  // 两个连续节点构成的区域
	CTNTYPE_SELF_LOOP,  // 自循环
	CTNTYPE_IF,
	CTNTYPE_IF_ELSE,
	CTNTYPE_IF_OR,  // if else 内部多出一条边
	CTNTYPE_P2LOOP
};

const TCHAR* ToString(CtrlTreeNodeType region);

struct ControlTreeNode : BasicBlock
{
	CtrlTreeNodeType type;
	// 不同类型对应不同的字段
	union
	{
		// 自循环的区域
		ControlTreeNode* node;
		// 连续两个区域 或 2点循环
		struct
		{
			ControlTreeNode* first;
			ControlTreeNode* second;
		} pair;
		// if 区域
		struct
		{
			ControlTreeNode* condition;
			ControlTreeNode* then;
			ControlTreeNode* _else;
		} _if;
	};
};