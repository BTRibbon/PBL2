#pragma once

#include "../models/Order.h"
#include <queue>

class OrderQueue {
	std::queue<Order> q;
public:
	void push(Order order);
	Order popNext();
	bool empty() const;
	int size() const;
};
