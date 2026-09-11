#pragma once

#include "../models/MenuItem.h"
#include <string>
#include <unordered_map>

class MenuLookup {
	std::unordered_map<std::string,MenuItem> table;
public:
	void addItem(MenuItem item);
	MenuItem* find(std::string id);
};
