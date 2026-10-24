#pragma once

#include "../datastructures/BranchList.h"
#include "../datastructures/String.h"
#include "../datastructures/Vector.h"

class RestaurantChain {
private:
	String name;
	BranchList branches;

public:
	explicit RestaurantChain(const String& chainName);
	void addBranch(const String& id, const String& branchName, float x, float y);
	bool removeBranch(const String& id);
	Branch* findBranch(const String& id);
	Branch* findNearestBranch(float x, float y);
	Vector<Branch> getBranches() const;
	String getName() const;
	int getBranchCount() const { return branches.size(); }
	void clear() { branches.clear(); }
};
