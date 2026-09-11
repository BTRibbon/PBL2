#include "RestaurantChain.h"

RestaurantChain::RestaurantChain(std::string chainName) : name(chainName) {}

void RestaurantChain::addBranch(std::string id,std::string branchName,float x,float y) { branches.addBranch(id,branchName,x,y); }

Branch* RestaurantChain::findNearestBranch(float x,float y) { return branches.findNearest(x,y); }

std::vector<Branch> RestaurantChain::getBranches() const { return branches.values(); }

std::string RestaurantChain::getName() const { return name; }
