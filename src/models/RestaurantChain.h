#pragma once

#include "../datastructures/BranchList.h"
#include <string>
#include <vector>

class RestaurantChain {
	std::string name;
	BranchList branches;
public:
	RestaurantChain(std::string chainName);
	void addBranch(std::string id,std::string branchName,float x,float y);
	Branch* findNearestBranch(float x,float y);
	std::vector<Branch> getBranches() const;
	std::string getName() const;
};
