#include "MenuLookup.h"

#include <cstdint>

MenuLookup::Node::Node(const String& k, const MenuItem& v, Node* following)
	: key(k), item(v), next(following) {}

int MenuLookup::hashKey(const String& key) const {
	// Hàm băm FNV-1a 32-bit trên chuỗi mã món
	std::uint32_t hash = 2166136261u;
	for (std::size_t i = 0; i < key.size(); ++i) {
		hash ^= static_cast<unsigned char>(key[i]);
		hash *= 16777619u;
	}
	return static_cast<int>(hash % static_cast<std::uint32_t>(BUCKET_COUNT));
}

void MenuLookup::copyFrom(const MenuLookup& other) {
	for (int i = 0; i < BUCKET_COUNT; ++i) {
		buckets_[i] = nullptr;
	}
	count_ = 0;
	for (int i = 0; i < BUCKET_COUNT; ++i) {
		for (Node* curr = other.buckets_[i]; curr != nullptr; curr = curr->next) {
			addItem(curr->item);
		}
	}
}

MenuLookup::MenuLookup() : count_(0) {
	for (int i = 0; i < BUCKET_COUNT; ++i) {
		buckets_[i] = nullptr;
	}
}

MenuLookup::MenuLookup(const MenuLookup& other) : count_(0) {
	copyFrom(other);
}

MenuLookup& MenuLookup::operator=(const MenuLookup& other) {
	if (this == &other) return *this;
	clear();
	copyFrom(other);
	return *this;
}

MenuLookup::~MenuLookup() {
	clear();
}

void MenuLookup::addItem(const MenuItem& item) {
	int idx = hashKey(item.id);
	for (Node* curr = buckets_[idx]; curr != nullptr; curr = curr->next) {
		if (curr->key == item.id) {
			curr->item = item;
			return;
		}
	}
	buckets_[idx] = new Node(item.id, item, buckets_[idx]);
	++count_;
}

bool MenuLookup::removeItem(const String& id) {
	int idx = hashKey(id);
	Node* prev = nullptr;
	Node* curr = buckets_[idx];
	while (curr != nullptr) {
		if (curr->key == id) {
			if (prev) prev->next = curr->next;
			else buckets_[idx] = curr->next;
			delete curr;
			--count_;
			return true;
		}
		prev = curr;
		curr = curr->next;
	}
	return false;
}

MenuItem* MenuLookup::find(const String& id) {
	int idx = hashKey(id);
	for (Node* curr = buckets_[idx]; curr != nullptr; curr = curr->next) {
		if (curr->key == id) {
			return &curr->item;
		}
	}
	return nullptr;
}

const MenuItem* MenuLookup::find(const String& id) const {
	int idx = hashKey(id);
	for (Node* curr = buckets_[idx]; curr != nullptr; curr = curr->next) {
		if (curr->key == id) {
			return &curr->item;
		}
	}
	return nullptr;
}

void MenuLookup::clear() {
	for (int i = 0; i < BUCKET_COUNT; ++i) {
		Node* curr = buckets_[i];
		while (curr != nullptr) {
			Node* old = curr;
			curr = curr->next;
			delete old;
		}
		buckets_[i] = nullptr;
	}
	count_ = 0;
}
