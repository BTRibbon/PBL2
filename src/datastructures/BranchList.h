#pragma once

#include "../models/Branch.h"
#include "String.h"
#include "Vector.h"

// Cấu trúc dữ liệu Danh sách liên kết đơn (Singly Linked List) tự cài đặt
// Quản lý danh sách chi nhánh của chuỗi nhà hàng, tuân thủ Rule of Three
class BranchList {
private:
	struct Node {
		Branch branch;
		Node* next;
		Node(const Branch& value, Node* following = nullptr);
	};

	Node* head_ = nullptr;
	Node* tail_ = nullptr;
	int count_ = 0;

	void copyFrom(const BranchList& other);

public:
	BranchList();
	BranchList(const BranchList& other);
	BranchList& operator=(const BranchList& other);
	~BranchList();

	void add(const Branch& branch);
	void addBranch(const String& id, const String& name, float x, float y);
	bool removeById(const String& id);
	Branch* findById(const String& id);
	Branch* findNearest(float x, float y);
	Vector<Branch> values() const;
	int size() const { return count_; }
	bool empty() const { return head_ == nullptr; }
	void clear();
};
