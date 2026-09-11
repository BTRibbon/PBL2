#pragma once

#include "OpeningHours.h"
#include "Warehouse.h"
#include "MenuItem.h"
#include "../datastructures/MenuLookup.h"
#include <string>

class Branch {
	std::string branchId;
	std::string name;
	float x;
	float y;
	MenuLookup menu;
	Warehouse warehouse;
	OpeningHours openingHours;
	int revenue=0;
public:
	Branch(std::string id,std::string branchName,float branchX,float branchY);
	std::string getId() const;
	std::string getName() const;
	float getX() const;
	float getY() const;
	void addMenuItem(MenuItem item);
	MenuItem* findMenuItem(std::string itemId);
	void addStock(std::string itemId,int quantity);
	int getStock(std::string itemId) const;
	void setOpeningHours(std::string open,std::string close);
	OpeningHours getOpeningHours() const;
	void addRevenue(int amount);
	int getRevenue() const;
};
