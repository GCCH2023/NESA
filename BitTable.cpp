#include "stdafx.h"
#include "BitTable.h"

BitTable::BitTable() :
rows(0),
columns(0),
stride(0)
{

}

BitTable::BitTable(size_t rows, size_t columns)
{
	// 计算需要的 size_t 数量
	const size_t mask = BLOCK_TYPE_BITS - 1;
	size_t columnBits = (columns + mask) & ~mask;
	stride = columnBits / BLOCK_TYPE_BITS;
	data.resize(rows * stride);
	this->rows = rows;
	this->columns = columns;
}

bool BitTable::IsRowAll1(size_t row)
{
	const BlockType* rowData = Row(row);
	const size_t fullBlocks = columns / BLOCK_TYPE_BITS;
	const size_t remainingBits = columns % BLOCK_TYPE_BITS;

	// 检查完整的 BlockType 块是否全为 1
	for (size_t i = 0; i < fullBlocks; ++i) {
		if (rowData[i] != ~BlockType(0)) // ~0 表示全 1
			return false;
	}

	// 检查剩余位是否全为 1
	if (remainingBits > 0) {
		const BlockType mask = (BlockType(1) << remainingBits) - 1;
		if ((rowData[fullBlocks] & mask) != mask)
			return false;
	}

	return true;
}

bool BitTable::IsRowAll0(size_t row)
{
	const BlockType* rowData = Row(row);
	for (size_t i = 0; i < stride; ++i)
	{
		if (rowData[i] != 0)
			return false;
	}
	return true;
}

void BitTable::RowSet(size_t target, size_t source)
{
	memcpy_s(Row(source), stride * sizeof(BlockType), Row(target), stride * sizeof(BlockType));
}

void BitTable::RowAnd(size_t target, size_t source)
{
	auto dest = Row(target);
	const auto src = Row(source);
	for (size_t i = 0; i < stride; ++i)
		dest[i] &= src[i];
}

void BitTable::RowOr(size_t target, size_t source)
{
	auto dest = Row(target);
	const auto src = Row(source);
	for (size_t i = 0; i < stride; ++i)
		dest[i] |= src[i];
}

bool BitTable::Get(size_t row, size_t column)
{
	CheckColumn(column);
	auto r = Row(row);
	return (r[column / BLOCK_TYPE_BITS] >> (column % BLOCK_TYPE_BITS)) != 0;
}

void BitTable::Set(size_t row, size_t column, bool value)
{
	CheckColumn(column);
	auto r = Row(row);

	BlockType bit = column % BLOCK_TYPE_BITS;
	if (value)
		r[column / BLOCK_TYPE_BITS] |= (BlockType(1) << bit);
	else
		r[column / BLOCK_TYPE_BITS] &= ~(BlockType(1) << bit);
}
