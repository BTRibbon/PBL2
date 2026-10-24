#pragma once

#include "../models/Order.h"

// Cấu trúc dữ liệu Hàng đợi (FIFO Queue) tự cài đặt bằng Danh sách liên kết đơn
// Quản lý hàng đợi đơn hàng chờ xử lý: thêm vào cuối (rear) O(1), lấy ra ở đầu (front) O(1)
class OrderQueue {
private:
	struct Node {
		Order order;
		Node* next;
		Node(const Order& value, Node* following = nullptr);
	};

	Node* front_;
	Node* rear_;
	int count_;

	void copyFrom(const OrderQueue& other);

public:
	OrderQueue();
	OrderQueue(const OrderQueue& other);
	OrderQueue& operator=(const OrderQueue& other);
	~OrderQueue();

	void push(const Order& order);
	Order popNext();
	const Order* peek() const;
	bool empty() const;
	int size() const;
	void clear();
};
