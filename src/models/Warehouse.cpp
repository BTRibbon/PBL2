#include "Warehouse.h"

void Warehouse::addStock(std::string itemId,int quantity) { stock[itemId]+=quantity; }

bool Warehouse::takeStock(std::string itemId,int quantity) {
	if (getStock(itemId)<quantity) return false;
	stock[itemId]-=quantity;
	return true;
}

int Warehouse::getStock(std::string itemId) const {
	auto it=stock.find(itemId);
	return it!=stock.end() ? it->second : 0;
}
