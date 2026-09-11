#include "BranchList.h"

#include <algorithm>
#include <limits>

BranchList::Node::Node(const Branch& value,Node* following) : branch(value),next(following) {}

BranchList::~BranchList() { clear(); }

void BranchList::add(const Branch& branch) { head=new Node(branch,head); }

void BranchList::addBranch(std::string id,std::string name,float x,float y) { add(Branch(id,name,x,y)); }

Branch* BranchList::findNearest(float x,float y) {
	Branch* best=nullptr;
	float bestDistance=std::numeric_limits<float>::max();
	for (Node* node=head;node;node=node->next) {
		float dx=node->branch.getX()-x;
		float dy=node->branch.getY()-y;
		float distance=dx*dx+dy*dy;
		if (distance<bestDistance) { bestDistance=distance;best=&node->branch; }
	}
	return best;
}

std::vector<Branch> BranchList::values() const {
	std::vector<Branch> result;
	for (Node* node=head;node;node=node->next) result.push_back(node->branch);
	std::reverse(result.begin(),result.end());
	return result;
}

void BranchList::clear() {
	while (head) { Node* old=head;head=head->next;delete old; }
}
