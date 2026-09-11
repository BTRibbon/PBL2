#pragma once

#include "../models/Branch.h"
#include <vector>

class BranchList {
	struct Node {
		Branch branch;
		Node* next;
		Node(const Branch& value,Node* following);
	};
	Node* head=nullptr;
public:
	~BranchList();
	void add(const Branch& branch);
	void addBranch(std::string id,std::string name,float x,float y);
	Branch* findNearest(float x,float y);
	std::vector<Branch> values() const;
	void clear();
};
