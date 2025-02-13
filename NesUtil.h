#pragma once

// ������� NES ����
template<typename T>
void AddNesObject(std::vector<T*>& list, T* obj)
{
	if (!obj)
		return;

	auto it = std::lower_bound(list.begin(), list.end(), obj,
		[](T* a, T* b){
		return a->GetStartAddress() < b->GetStartAddress();
	});
	// ����λ���Ƿ��Ѿ�������ͬ��ֵ
	if (it == list.end() || (*it)->GetStartAddress() != obj->GetStartAddress())
		// �����������ͬ��ֵ���������ֵ
		list.insert(it, obj);
}

// ���ݵ�ַ���� NES ����
template<typename T>
T* FindNesObject(std::vector<T*>& list, Nes::Address address)
{
	auto it = std::lower_bound(list.begin(), list.end(), address,
		[](T* a, Nes::Address address){
		return a->GetStartAddress() < address;
	});
	// ����λ���Ƿ��Ѿ�������ͬ��ֵ
	if (it != list.end())
		return *it;
	return nullptr;
}
