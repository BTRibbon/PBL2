#pragma once

#include "raylib.h"
#include "../datastructures/String.h"
#include "../datastructures/Vector.h"
#include "../datastructures/MenuLookup.h"
#include "../datastructures/OrderQueue.h"
#include "../models/Branch.h"
#include "../models/Employee.h"
#include "../models/Order.h"
#include "../services/DataManager.h"

struct CartItem {
	String productCode;
	int quantity = 0;

	CartItem() = default;
	CartItem(const String& code, int qty) : productCode(code), quantity(qty) {}
};

struct CartFlyer {
	Vector2 from;
	Vector2 to;
	double start = 0.0;
	Color color{};
};

// Giao diện Tab 0: Bán hàng tại quầy / Kiosk (POS) & Hàng đợi đơn hàng (OrderQueue)
class PosUI {
private:
	Vector<CartItem> cart;
	String search;
	bool searchFocused = false;
	String categoryFilter = "Tất cả";

	int posScrollRowOffset = 0;
	int cartScrollOffset = 0;

	int discountPercent = 0;
	bool comboPromotion = false;
	String discountInputText = "0";
	bool discountInputFocused = false;

	bool orderPlaced = false;
	int nextOrderId = 1001;
	float displayedTotal = 0.0f;
	String cartFlashCode;
	double cartFlashTime = -10.0;
	Vector<CartFlyer> cartFlyers;

	Vector<int> buildFilteredIndices(const Vector<Product>& products) const;
	static const Product* findProduct(const Vector<Product>& products, const String& code);
	static int cartTotal(const Vector<CartItem>& cartItems, const Vector<Product>& products, const MenuLookup& menuLookup);
	static int computeDiscount(const Vector<CartItem>& cartItems, const Vector<Product>& products,
		const MenuLookup& menuLookup, int discPercent, bool comboEnabled);

public:
	PosUI() = default;

	void syncNextOrderId(const Vector<Order>& orders);
	bool isInputFocused() const { return searchFocused || discountInputFocused; }

	void handleScroll(Vector2 mouse, float wheel, const Vector<Product>& products);
	void handleInput(int keyPress, int charPressed);
	bool handleClick(Vector2 mouse, const Vector<Product>& products, const MenuLookup& menuLookup,
		const Vector<Branch>& branches, Vector<Order>& orders, OrderQueue& orderQueue,
		const Employee* currentUser, String& statusMessage, int& statusVersion);

	void draw(Vector2 uiMouse, Vector2 rawMouse, const Vector<Product>& products,
		const MenuLookup& menuLookup, const OrderQueue& orderQueue, double statusChangeTime);
};

