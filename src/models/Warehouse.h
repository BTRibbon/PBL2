#pragma once

#include <string>
#include <unordered_map>

class Warehouse {
	std::unordered_map<std::string,int> stock;
public:
	void addStock(std::string itemId,int quantity);
	bool takeStock(std::string itemId,int quantity);
	bool removeStock(std::string itemId,int quantity);
	int getStock(std::string itemId) const;
};
