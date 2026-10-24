#include "Branch.h"

Branch::Branch(const String& id, const String& branchName, float branchX, float branchY)
	: branchId(id), name(branchName), x(branchX), y(branchY) {}

String Branch::getId() const { return branchId; }

String Branch::getName() const { return name; }

float Branch::getX() const { return x; }

float Branch::getY() const { return y; }

void Branch::addMenuItem(const MenuItem& item) { menu.addItem(item); }

MenuItem* Branch::findMenuItem(const String& itemId) { return menu.find(itemId); }

void Branch::addStock(const String& itemId, int quantity) { warehouse.addStock(itemId, quantity); }

bool Branch::removeStock(const String& itemId, int quantity) { return warehouse.removeStock(itemId, quantity); }

int Branch::getStock(const String& itemId) const { return warehouse.getStock(itemId); }

void Branch::setOpeningHours(const String& open, const String& close) { openingHours.setHours(open, close); }

OpeningHours Branch::getOpeningHours() const { return openingHours; }

void Branch::addRevenue(int amount) { revenue += amount; }

int Branch::getRevenue() const { return revenue; }
