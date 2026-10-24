#include "Warehouse.h"

void Warehouse::addStock(const String& itemId, int quantity) {
	if (quantity <= 0) return;
	for (int i = 0; i < stock.size(); ++i) {
		if (stock[i].itemId == itemId) {
			stock[i].quantity += quantity;
			return;
		}
	}
	stock.push_back(StockEntry(itemId, quantity));
}

bool Warehouse::takeStock(const String& itemId, int quantity) {
	if (quantity <= 0) return false;
	for (int i = 0; i < stock.size(); ++i) {
		if (stock[i].itemId == itemId) {
			if (stock[i].quantity < quantity) return false;
			stock[i].quantity -= quantity;
			return true;
		}
	}
	return false;
}

bool Warehouse::removeStock(const String& itemId, int quantity) {
	return takeStock(itemId, quantity);
}

int Warehouse::getStock(const String& itemId) const {
	for (int i = 0; i < stock.size(); ++i) {
		if (stock[i].itemId == itemId) {
			return stock[i].quantity;
		}
	}
	return 0;
}
