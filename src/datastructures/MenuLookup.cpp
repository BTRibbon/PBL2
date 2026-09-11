#include "MenuLookup.h"

void MenuLookup::addItem(MenuItem item) { table[item.id]=item; }

MenuItem* MenuLookup::find(std::string id) {
	auto it=table.find(id);
	return it!=table.end() ? &it->second : nullptr;
}
