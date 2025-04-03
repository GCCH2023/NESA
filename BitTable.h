#pragma once
#include <vector>

// 二维位表
// 在运算过程中，需要保证没用到的位始终为0
class BitTable
{
public:
	using BlockType = size_t;
#define BLOCK_TYPE_BITS (sizeof(BlockType) * CHAR_BIT)

	BitTable();
	BitTable(size_t rows, size_t columns);
	~BitTable(){}
	// 判断指定行的所有列是否都是1
	bool IsRowAll1(size_t row);
	// 判断指定行的所有列是否都是0
	bool IsRowAll0(size_t row);
	// 使用指定行来给另一行赋值
	void RowSet(size_t target, size_t source);
	// 指定两行相与
	void RowAnd(size_t target, size_t source);
	// 指定两行相或
	void RowOr(size_t target, size_t source);
	// 获取指定行列的值
	bool Get(size_t row, size_t column);
	// 设置指定行列的值
	void Set(size_t row, size_t column, bool value);
protected:
	// 如果行索引超出范围，抛出异常
	inline void CheckRow(size_t row) const
	{
		if (row >= rows)
			throw std::out_of_range("Row index out of range");
	}
	// 如果列索引超出范围，抛出异常
	inline void CheckColumn(size_t column) const
	{
		if (column >= columns)
			throw std::out_of_range("Column index out of range");
	}
	// 获取指定行的数据指针
	inline BlockType* Row(size_t row)
	{
		CheckRow(row);
		return &data[row * stride];
	}
	// 获取指定行的数据指针
	inline const BlockType* Row(size_t row) const
	{
		CheckRow(row);
		return &data[row * stride];
	}
private:
	size_t rows;  // 行数
	size_t columns;  // 列数
	size_t stride;  // 每行占用多少个 size_t
	std::vector<BlockType> data;
};

