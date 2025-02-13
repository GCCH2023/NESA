#pragma once

// 有序添加 NES 对象
template<typename T>
void AddNesObject(std::vector<T*>& list, T* obj)
{
	if (!obj)
		return;

	auto it = std::lower_bound(list.begin(), list.end(), obj,
		[](T* a, T* b){
		return a->GetStartAddress() < b->GetStartAddress();
	});
	// 检查该位置是否已经存在相同的值
	if (it == list.end() || (*it)->GetStartAddress() != obj->GetStartAddress())
		// 如果不存在相同的值，则插入新值
		list.insert(it, obj);
}

// 根据地址查找 NES 对象
template<typename T>
T* FindNesObject(std::vector<T*>& list, Nes::Address address)
{
	auto it = std::lower_bound(list.begin(), list.end(), address,
		[](T* a, Nes::Address address){
		return a->GetStartAddress() < address;
	});
	// 检查该位置是否已经存在相同的值
	if (it != list.end())
		return *it;
	return nullptr;
}
