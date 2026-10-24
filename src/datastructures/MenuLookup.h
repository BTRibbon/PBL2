#pragma once

#include "../models/MenuItem.h"
#include "String.h"

// Cấu trúc dữ liệu Bảng băm (Hash Table) tự cài đặt theo phương pháp Nối kết riêng (Separate Chaining)
// Tra cứu thông tin món ăn theo mã món (id) với độ phức tạp trung bình O(1)
class MenuLookup {
private:
	static constexpr int BUCKET_COUNT = 31;

	struct Node {
		String key;
		MenuItem item;
		Node* next;
		Node(const String& k, const MenuItem& v, Node* following = nullptr);
	};

	Node* buckets_[BUCKET_COUNT];
	int count_;

	int hashKey(const String& key) const;
	void copyFrom(const MenuLookup& other);

public:
	MenuLookup();
	MenuLookup(const MenuLookup& other);
	MenuLookup& operator=(const MenuLookup& other);
	~MenuLookup();

	void addItem(const MenuItem& item);
	bool removeItem(const String& id);
	MenuItem* find(const String& id);
	const MenuItem* find(const String& id) const;
	void clear();
	int size() const { return count_; }
};
