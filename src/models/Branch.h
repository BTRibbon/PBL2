#pragma once

#include "OpeningHours.h"
#include "Warehouse.h"
#include "MenuItem.h"
#include "../datastructures/MenuLookup.h"
#include "../datastructures/String.h"

class Branch {
private:
	String branchId;
	String name;
	float x = 0.0f;
	float y = 0.0f;
	MenuLookup menu;
	Warehouse warehouse;
	OpeningHours openingHours;
	int revenue = 0;

public:
	Branch() = default;
	Branch(const String& id, const String& branchName, float branchX, float branchY);

	String getId() const;
	String getName() const;
	float getX() const;
	float getY() const;

	void addMenuItem(const MenuItem& item);
	MenuItem* findMenuItem(const String& itemId);
	void addStock(const String& itemId, int quantity);
	bool removeStock(const String& itemId, int quantity);
	int getStock(const String& itemId) const;
	void setOpeningHours(const String& open, const String& close);
	OpeningHours getOpeningHours() const;
	void addRevenue(int amount);
	int getRevenue() const;

	bool operator==(const Branch& other) const {
		return branchId == other.branchId;
	}
	bool operator!=(const Branch& other) const {
		return branchId != other.branchId;
	}
};
