#include "RestaurantChain.h"

RestaurantChain::RestaurantChain(const String& chainName) : name(chainName) {}

void RestaurantChain::addBranch(const String& id, const String& branchName, float x, float y) {
	branches.addBranch(id, branchName, x, y);
}

bool RestaurantChain::removeBranch(const String& id) {
	return branches.removeById(id);
}

Branch* RestaurantChain::findBranch(const String& id) {
	return branches.findById(id);
}

Branch* RestaurantChain::findNearestBranch(float x, float y) {
	return branches.findNearest(x, y);
}

Vector<Branch> RestaurantChain::getBranches() const {
	return branches.values();
}

String RestaurantChain::getName() const {
	return name;
}
