#include "OrderQueue.h"

OrderQueue::Node::Node(const Order& value, Node* following)
	: order(value), next(following) {}

OrderQueue::OrderQueue() : front_(nullptr), rear_(nullptr), count_(0) {}

void OrderQueue::copyFrom(const OrderQueue& other) {
	front_ = nullptr;
	rear_ = nullptr;
	count_ = 0;
	for (Node* curr = other.front_; curr != nullptr; curr = curr->next) {
		push(curr->order);
	}
}

OrderQueue::OrderQueue(const OrderQueue& other)
	: front_(nullptr), rear_(nullptr), count_(0) {
	copyFrom(other);
}

OrderQueue& OrderQueue::operator=(const OrderQueue& other) {
	if (this == &other) return *this;
	clear();
	copyFrom(other);
	return *this;
}

OrderQueue::~OrderQueue() {
	clear();
}

void OrderQueue::push(const Order& order) {
	Node* newNode = new Node(order, nullptr);
	if (!rear_) {
		front_ = newNode;
		rear_ = newNode;
	} else {
		rear_->next = newNode;
		rear_ = newNode;
	}
	++count_;
}

Order OrderQueue::popNext() {
	if (!front_) return Order{};
	Node* oldFront = front_;
	Order result = oldFront->order;
	front_ = front_->next;
	if (!front_) {
		rear_ = nullptr;
	}
	delete oldFront;
	--count_;
	return result;
}

const Order* OrderQueue::peek() const {
	return front_ ? &front_->order : nullptr;
}

bool OrderQueue::empty() const {
	return front_ == nullptr;
}

int OrderQueue::size() const {
	return count_;
}

void OrderQueue::clear() {
	while (front_) {
		Node* old = front_;
		front_ = front_->next;
		delete old;
	}
	rear_ = nullptr;
	count_ = 0;
}
