#include "Branch.h"

Branch::Branch(std::string id,std::string branchName,float branchX,float branchY) : branchId(id),name(branchName),x(branchX),y(branchY) {}

std::string Branch::getId() const { return branchId; }

std::string Branch::getName() const { return name; }

float Branch::getX() const { return x; }

float Branch::getY() const { return y; }

void Branch::addMenuItem(MenuItem item) { menu.addItem(item); }

MenuItem* Branch::findMenuItem(std::string itemId) { return menu.find(itemId); }

void Branch::addStock(std::string itemId,int quantity) { warehouse.addStock(itemId,quantity); }

int Branch::getStock(std::string itemId) const { return warehouse.getStock(itemId); }

void Branch::setOpeningHours(std::string open,std::string close) { openingHours.setHours(open,close); }

OpeningHours Branch::getOpeningHours() const { return openingHours; }

void Branch::addRevenue(int amount) { revenue+=amount; }

int Branch::getRevenue() const { return revenue; }
