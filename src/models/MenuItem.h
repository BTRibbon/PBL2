#pragma once

#include "../datastructures/String.h"

struct MenuItem {
	String id;
	String name;
	float price = 0.0f;

	MenuItem() = default;
	MenuItem(const String& itemId, const String& itemName, float itemPrice)
		: id(itemId), name(itemName), price(itemPrice) {}

	bool operator==(const MenuItem& other) const {
		return id == other.id;
	}
	bool operator!=(const MenuItem& other) const {
		return id != other.id;
	}
};
