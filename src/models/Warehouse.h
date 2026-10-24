#pragma once

#include "../datastructures/String.h"
#include "../datastructures/Vector.h"

struct StockEntry {
	String itemId;
	int quantity = 0;

	StockEntry() = default;
	StockEntry(const String& id, int qty) : itemId(id), quantity(qty) {}
};

class Warehouse {
private:
	Vector<StockEntry> stock;

public:
	void addStock(const String& itemId, int quantity);
	bool takeStock(const String& itemId, int quantity);
	bool removeStock(const String& itemId, int quantity);
	int getStock(const String& itemId) const;
};
