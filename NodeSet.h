#pragma once

// 32 λ ��λ��
class BitSet32
{
public:
	BitSet32();
	BitSet32(uint32_t value);
	BitSet32(std::initializer_list<int> values);
	// ��ȡλ����Ϊ1��λ������
	std::vector<int> ToVector() const;
	// ��ȡλ����������ʾ
	inline uint32_t ToInteger() const { return data; }
	// ��ȡλ����1�ĸ���
	int GetSize() const;
	// ������һ��λ��
	inline BitSet32& operator|=(const BitSet32 other) { data |= other.data; return *this; }
	// ��ȡ����λ���Ĳ���
	inline BitSet32 operator|(const BitSet32 other) const { return BitSet32(data | other.data); }
	// ����һ��λ���ཻ
	inline BitSet32& operator&=(const BitSet32 other) { data &= other.data; return *this; }
	// ��ȡ����λ���Ľ���
	inline BitSet32 operator&(const BitSet32 other) const { return BitSet32(data & other.data); }
	// ��λ������ȡ��������Ҳ�����󲹼�
	inline BitSet32 operator~() const { return BitSet32(~data); }
	// ���һ��Ԫ��
	inline BitSet32& operator+=(int index) { data |= 1 << index; return *this; }
	// ���ָ��λ���е�Ԫ��
	inline BitSet32& operator+=(const BitSet32& a) { data |= a.data; return *this; }
	// �Ƴ�һ��Ԫ��
	inline BitSet32& operator-=(int index) { data &= ~(1 << index); return *this; }
	// �Ƴ�ָ��λ���е�Ԫ��
	inline BitSet32& operator-=(const BitSet32& a) { data &= ~a.data; return *this; }
	// ��ȡ�Ƴ�һ��Ԫ�غ��λ��
	inline BitSet32 operator-(int index) const { return BitSet32(data & ~(1 << index)); }
	// �����ϸ�ֵ
	inline BitSet32& operator=(uint32_t value) { data = value; return *this; }
	// �ж�����λ���Ƿ����
	inline bool operator==(const BitSet32 other) { return data == other.data; }
	inline bool operator==(uint32_t value) { return data == value; }
	// �ж�����λ���Ƿ񲻵�
	inline bool operator!=(const BitSet32 other) { return data != other.data; }
	inline bool operator!=(uint32_t value) { return data != value; }
	// λ�����Ƿ����ָ��Ԫ��
	inline bool Contains(int index) const { return (data & (1 << index)) != 0; }
	// �滻λ���е� a Ԫ��Ϊ b Ԫ��
	inline BitSet32& Replace(int a, int b) { *this -= a; *this += b; return *this; }
	// �滻λ���е���λ�� a Ϊָ��λ�� b
	inline BitSet32& Replace(const BitSet32& a, const BitSet32& b)
	{
		data = data & ~a.data | b.data;
		return *this;
	}
	// �Ƿ������Ԫ��
	inline bool Any() const { return data != 0; }
	// �Ƿ�Ϊ��
	inline bool None() const { return data == 0; }
	// ����ַ�����ʾ
	void Dump() const;
private:
	uint32_t data;
};

using Node = int;
using NodeSet = BitSet32;

// ��ȡλ����Ӧ�Ľڵ��б�
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
	BitSet32 pred;  // ǰ���ڵ㼯
	BitSet32 succ;  // ��̽ڵ㼯
	BitSet32 domin;  // �ؾ��ڵ㼯

	// ��ȡ�������
	inline int GetSuccCount() const { return succ.GetSize(); }
	// ��ȡǰ������
	inline int GetPredCount() const { return pred.GetSize(); }
	// ��ȡ����ǰ��������������б�
	inline std::vector<Node> Pred() const { return pred.ToVector(); }
	// ��ȡ���к�̻�����������б�
	inline std::vector<Node> Succ() const { return succ.ToVector(); }
	// ��ȡָ���ڵ�����ڣ�ǰ��+��̣��ڵ㼯��
	inline std::vector<Node> Adjacent()
	{
		BitSet32 total = pred | succ;
		return total.ToVector();
	}
	// ����ַ�����ʾ
	void Dump();
};

enum CtrlTreeNodeType
{
	CTNTYPE_LEAF,  // Ҷ������
	CTNTYPE_LIST,  // ���������ڵ㹹�ɵ�����
	CTNTYPE_SELF_LOOP,  // ��ѭ��
	CTNTYPE_IF,
	CTNTYPE_IF_ELSE,
	CTNTYPE_IF_OR,  // if else �ڲ����һ����
	CTNTYPE_P2LOOP
};

const TCHAR* ToString(CtrlTreeNodeType region);

struct ControlTreeNode : BasicBlock
{
	CtrlTreeNodeType type;
	// ��ͬ���Ͷ�Ӧ��ͬ���ֶ�
	union
	{
		// ��ѭ��������
		ControlTreeNode* node;
		// ������������ �� 2��ѭ��
		struct
		{
			ControlTreeNode* first;
			ControlTreeNode* second;
		} pair;
		// if ����
		struct
		{
			ControlTreeNode* condition;
			ControlTreeNode* then;
			ControlTreeNode* _else;
		} _if;
	};
};