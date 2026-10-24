#include "BranchList.h"

BranchList::Node::Node(const Branch& value, Node* following)
	: branch(value), next(following) {}

BranchList::BranchList() : head_(nullptr), tail_(nullptr), count_(0) {}

void BranchList::copyFrom(const BranchList& other) {
	head_ = nullptr;
	tail_ = nullptr;
	count_ = 0;
	for (Node* curr = other.head_; curr != nullptr; curr = curr->next) {
		add(curr->branch);
	}
}

BranchList::BranchList(const BranchList& other)
	: head_(nullptr), tail_(nullptr), count_(0) {
	copyFrom(other);
}

BranchList& BranchList::operator=(const BranchList& other) {
	if (this == &other) return *this;
	clear();
	copyFrom(other);
	return *this;
}

BranchList::~BranchList() {
	clear();
}

void BranchList::add(const Branch& branch) {
	Node* newNode = new Node(branch, nullptr);
	if (!tail_) {
		head_ = newNode;
		tail_ = newNode;
	} else {
		tail_->next = newNode;
		tail_ = newNode;
	}
	++count_;
}

void BranchList::addBranch(const String& id, const String& name, float x, float y) {
	add(Branch(id, name, x, y));
}

bool BranchList::removeById(const String& id) {
	Node* prev = nullptr;
	Node* curr = head_;
	while (curr != nullptr) {
		if (curr->branch.getId() == id) {
			if (prev) prev->next = curr->next;
			else head_ = curr->next;
			if (curr == tail_) tail_ = prev;
			delete curr;
			--count_;
			return true;
		}
		prev = curr;
		curr = curr->next;
	}
	return false;
}

Branch* BranchList::findById(const String& id) {
	for (Node* curr = head_; curr != nullptr; curr = curr->next) {
		if (curr->branch.getId() == id) {
			return &curr->branch;
		}
	}
	return nullptr;
}

Branch* BranchList::findNearest(float x, float y) {
	Branch* best = nullptr;
	float bestDistance = 1e30f;
	for (Node* node = head_; node != nullptr; node = node->next) {
		float dx = node->branch.getX() - x;
		float dy = node->branch.getY() - y;
		float distance = dx * dx + dy * dy;
		if (distance < bestDistance) {
			bestDistance = distance;
			best = &node->branch;
		}
	}
	return best;
}

Vector<Branch> BranchList::values() const {
	Vector<Branch> result;
	result.reserve(count_);
	for (Node* node = head_; node != nullptr; node = node->next) {
		result.push_back(node->branch);
	}
	return result;
}

void BranchList::clear() {
	while (head_) {
		Node* old = head_;
		head_ = head_->next;
		delete old;
	}
	tail_ = nullptr;
	count_ = 0;
}
