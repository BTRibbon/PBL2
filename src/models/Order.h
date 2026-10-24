#pragma once

#include "../datastructures/String.h"
#include "../datastructures/Vector.h"

struct OrderItem {
	String productCode;
	int quantity = 0;

	OrderItem() = default;
	OrderItem(const String& code, int qty) : productCode(code), quantity(qty) {}

	bool operator==(const OrderItem& other) const {
		return productCode == other.productCode && quantity == other.quantity;
	}
};

struct Order {
	int id = 0;
	int total = 0;
	String branch;
	String orderId;
	String customerId;
	Vector<OrderItem> items;
	String status = "pending";

	bool operator==(const Order& other) const {
		return id == other.id;
	}
	bool operator!=(const Order& other) const {
		return id != other.id;
	}
};
