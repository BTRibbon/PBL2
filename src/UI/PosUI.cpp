#include "PosUI.h"
#include "UIFont.h"
#include "UIAnim.h"
#include "rlgl.h"

#include <cctype>
#include <cmath>

namespace {
const Color darkGreen{35, 55, 53, 255};
const Color green{35, 105, 78, 255};
const Color muted{104, 115, 108, 255};
const Color amber{180, 110, 40, 255};
}

void PosUI::syncNextOrderId(const Vector<Order>& orders) {
	nextOrderId = 1001;
	for (int i = 0; i < orders.size(); ++i) {
		nextOrderId = maxVal(nextOrderId, orders[i].id + 1);
	}
}

const Product* PosUI::findProduct(const Vector<Product>& products, const String& code) {
	for (int i = 0; i < products.size(); ++i) {
		if (products[i].code == code) return &products[i];
	}
	return nullptr;
}

int PosUI::cartTotal(const Vector<CartItem>& cartItems, const Vector<Product>& products, const MenuLookup& menuLookup) {
	int total = 0;
	for (int i = 0; i < cartItems.size(); ++i) {
		const CartItem& item = cartItems[i];
		// Tra cứu đơn giá món ăn bằng Bảng băm MenuLookup O(1)
		const MenuItem* menuItem = menuLookup.find(item.productCode);
		if (menuItem) {
			total += static_cast<int>(menuItem->price) * item.quantity;
		} else {
			const Product* p = findProduct(products, item.productCode);
			if (p) total += p->price * item.quantity;
		}
	}
	return total;
}

int PosUI::computeDiscount(const Vector<CartItem>& cartItems, const Vector<Product>& products,
	const MenuLookup& menuLookup, int discPercent, bool comboEnabled) {
	int subtotal = cartTotal(cartItems, products, menuLookup);
	if (subtotal <= 0) return 0;
	int discount = 0;
	if (discPercent > 0) {
		discount += (subtotal * clampVal(discPercent, 0, 100)) / 100;
	}
	if (comboEnabled) {
		bool hasMain = false;
		bool hasDrink = false;
		for (int i = 0; i < cartItems.size(); ++i) {
			const Product* p = findProduct(products, cartItems[i].productCode);
			if (p) {
				if (p->category == "Món chính") hasMain = true;
				if (p->category == "Đồ uống") hasDrink = true;
			}
		}
		if (hasMain && hasDrink) discount += 15000;
	}
	return minVal(discount, subtotal);
}

Vector<int> PosUI::buildFilteredIndices(const Vector<Product>& products) const {
	Vector<int> indices;
	String normalizedSearch = search.toLower();
	for (int i = 0; i < products.size(); ++i) {
		const Product& product = products[i];
		if (categoryFilter != "Tất cả" && product.category != categoryFilter) continue;
		if (!search.empty() &&
			product.name.toLower().find(normalizedSearch) == String::npos &&
			product.code.toLower().find(normalizedSearch) == String::npos) {
			continue;
		}
		indices.push_back(i);
	}
	return indices;
}

void PosUI::handleScroll(Vector2 mouse, float wheel, const Vector<Product>& products) {
	if (std::fabs(wheel) <= 0.01f) return;
	int step = (wheel > 0) ? -1 : 1;

	Vector<int> filtered = buildFilteredIndices(products);
	int totalPosRows = (filtered.size() + 1) / 2;
	int maxPosRowScroll = maxVal(0, totalPosRows - 4);
	int maxCartScroll = maxVal(0, cart.size() - 7);

	if (CheckCollisionPointRec(mouse, {935, 150, 320, 300})) {
		cartScrollOffset = clampVal(cartScrollOffset + step, 0, maxCartScroll);
	} else {
		posScrollRowOffset = clampVal(posScrollRowOffset + step, 0, maxPosRowScroll);
	}
}

void PosUI::handleInput(int keyPress, int charPressed) {
	if (discountInputFocused) {
		if (keyPress == KEY_BACKSPACE && !discountInputText.empty()) {
			discountInputText.pop_back();
			discountPercent = discountInputText.empty() ? 0 : clampVal(discountInputText.toInt(0), 0, 100);
		}
		if (std::isdigit(charPressed) && discountInputText.size() < 3) {
			if (discountInputText == "0") discountInputText.clear();
			discountInputText.push_back(static_cast<char>(charPressed));
			discountPercent = clampVal(discountInputText.toInt(0), 0, 100);
			discountInputText = toString(discountPercent);
		}
	} else if (searchFocused) {
		if (keyPress == KEY_BACKSPACE && !search.empty()) {
			PopUtf8Char(search);
			posScrollRowOffset = 0;
		}
		if (charPressed >= 32) {
			AppendUtf8Codepoint(search, charPressed, 30);
			posScrollRowOffset = 0;
		}
	}
}

bool PosUI::handleClick(Vector2 mouse, const Vector<Product>& products, const MenuLookup& menuLookup,
	const Vector<Branch>& branches, Vector<Order>& orders, OrderQueue& orderQueue,
	const Employee* currentUser, String& statusMessage, int& statusVersion) {

	Vector<int> filteredProductIndices = buildFilteredIndices(products);
	int totalPosRows = (filteredProductIndices.size() + 1) / 2;
	int maxPosRowScroll = maxVal(0, totalPosRows - 4);
	posScrollRowOffset = clampVal(posScrollRowOffset, 0, maxPosRowScroll);

	int maxCartScroll = maxVal(0, cart.size() - 7);
	cartScrollOffset = clampVal(cartScrollOffset, 0, maxCartScroll);

	searchFocused = CheckCollisionPointRec(mouse, {245, 148, 285, 36});

	// Nút xử lý đơn hàng chờ đầu hàng đợi FIFO (OrderQueue)
	if (!orderQueue.empty() && CheckCollisionPointRec(mouse, {490, 105, 195, 28})) {
		Order nextOrd = orderQueue.popNext();
		for (int i = 0; i < orders.size(); ++i) {
			if (orders[i].id == nextOrd.id) {
				orders[i].status = "completed";
				break;
			}
		}
		DataManager::saveOrders(orders);
		statusVersion++;
		statusMessage = String("Đã duyệt xong đơn hàng đầu hàng đợi #") + nextOrd.orderId + " (" + nextOrd.branch + ")!";
		return true;
	}

	const char* filterCategories[] = {"Tất cả", "Món chính", "Đồ uống", "Tráng miệng"};
	for (int c = 0; c < 4; ++c) {
		Rectangle catBtn = {540.0f + c * 88.0f, 148.0f, 82.0f, 36.0f};
		if (CheckCollisionPointRec(mouse, catBtn)) {
			categoryFilter = filterCategories[c];
			posScrollRowOffset = 0;
		}
	}

	if (totalPosRows > 4) {
		if (CheckCollisionPointRec(mouse, {895, 148, 28, 17})) {
			posScrollRowOffset = maxVal(0, posScrollRowOffset - 1);
		}
		if (CheckCollisionPointRec(mouse, {895, 167, 28, 17})) {
			posScrollRowOffset = minVal(maxPosRowScroll, posScrollRowOffset + 1);
		}
	}

	for (int slot = 0; slot < 8; ++slot) {
		int filteredIdx = posScrollRowOffset * 2 + slot;
		if (filteredIdx >= filteredProductIndices.size()) break;
		const Product& product = products[filteredProductIndices[filteredIdx]];

		int column = slot % 2;
		int row = slot / 2;
		Rectangle card = {245.0f + column * 336.0f, 200.0f + row * 106.0f, 322, 94};
		Rectangle addBtn = {card.x + 208, card.y + 54, 100, 30};

		if (CheckCollisionPointRec(mouse, addBtn) || CheckCollisionPointRec(mouse, card)) {
			orderPlaced = false;
			bool found = false;
			for (int i = 0; i < cart.size(); ++i) {
				if (cart[i].productCode == product.code) {
					cart[i].quantity++;
					found = true;
					break;
				}
			}
			if (!found) {
				cart.push_back(CartItem(product.code, 1));
				cartScrollOffset = maxVal(0, cart.size() - 7);
			}
			cartFlashCode = product.code;
			cartFlashTime = GetTime();
			{
				int cartIdx = 0;
				for (int ci = 0; ci < cart.size(); ++ci) {
					if (cart[ci].productCode == product.code) cartIdx = ci;
				}
				int visRow = clampVal(cartIdx - cartScrollOffset, 0, 6);
				cartFlyers.push_back(CartFlyer{
					{addBtn.x + addBtn.width / 2.0f, addBtn.y + addBtn.height / 2.0f},
					{1000.0f, 173.0f + visRow * 34.0f},
					GetTime(),
					product.color
				});
			}
			statusVersion++;
			statusMessage = String("Đã thêm vào giỏ: ") + product.name;
		}
	}

	for (int r = 0; r < 7; ++r) {
		int i = cartScrollOffset + r;
		if (i >= cart.size()) break;
		float rowY = 162.0f + r * 34.0f;
		Rectangle minusBtn = {1184, rowY, 22, 22};
		Rectangle plusBtn = {1210, rowY, 22, 22};
		if (CheckCollisionPointRec(mouse, minusBtn)) {
			cart[i].quantity--;
			if (cart[i].quantity <= 0) {
				cart.eraseAt(i);
			}
			break;
		}
		if (CheckCollisionPointRec(mouse, plusBtn)) {
			cart[i].quantity++;
			cartFlashCode = cart[i].productCode;
			cartFlashTime = GetTime();
			break;
		}
	}

	if (!cart.empty() && CheckCollisionPointRec(mouse, {1175, 116, 66, 26})) {
		cart.clear();
		cartScrollOffset = 0;
		statusVersion++;
		statusMessage = "Đã xoá toàn bộ giỏ hàng";
	}

	const int presets[] = {0, 10, 20, 30, 50};
	for (int p = 0; p < 5; ++p) {
		Rectangle pBtn = {952.0f + p * 58.0f, 478.0f, 52.0f, 26.0f};
		if (CheckCollisionPointRec(mouse, pBtn)) {
			discountPercent = presets[p];
			discountInputText = toString(discountPercent);
			discountInputFocused = false;
		}
	}

	Rectangle discBox = {1158, 510, 80, 26};
	discountInputFocused = CheckCollisionPointRec(mouse, discBox);

	Rectangle comboBtn = {952, 544, 286, 28};
	if (CheckCollisionPointRec(mouse, comboBtn)) {
		comboPromotion = !comboPromotion;
		statusVersion++;
		statusMessage = comboPromotion ? "Đã bật ưu đãi combo (-15.000 đ)" : "Đã tắt ưu đãi combo";
	}

	int totalAmount = cartTotal(cart, products, menuLookup);
	int discAmount = computeDiscount(cart, products, menuLookup, discountPercent, comboPromotion);
	int finalPaid = maxVal(0, totalAmount - discAmount);

	if (CheckCollisionPointRec(mouse, {952, 634, 286, 42}) && totalAmount <= 0) {
		statusVersion++;
		statusMessage = "Không thể thanh toán: giỏ hàng đang trống, vui lòng chọn món!";
	}
	if (CheckCollisionPointRec(mouse, {952, 634, 286, 42}) && totalAmount > 0) {
		orderPlaced = true;
		String orderBranch = branches.empty() ? String("Chi nhánh Bách Khoa") : branches.front().getName();
		if (currentUser && !currentUser->canManageAllBranches()) {
			for (int i = 0; i < branches.size(); ++i) {
				if (branches[i].getId() == currentUser->getBranchId()) {
					orderBranch = branches[i].getName();
					break;
				}
			}
		}
		Order order{};
		order.id = nextOrderId;
		order.total = finalPaid;
		order.branch = orderBranch;
		order.orderId = String("ORD") + toString(nextOrderId);
		order.customerId = "KIOSK";
		order.status = "pending";
		for (int i = 0; i < cart.size(); ++i) {
			order.items.push_back(OrderItem(cart[i].productCode, cart[i].quantity));
		}
		nextOrderId++;
		orderQueue.push(order);
		orders.push_back(order);
		if (DataManager::saveOrders(orders)) {
			statusVersion++;
			statusMessage = String("Đã thanh toán thành công đơn hàng #") + order.orderId + " (" + DataManager::formatCurrency(finalPaid) + ")!";
		} else {
			statusVersion++;
			statusMessage = "Đã tạo đơn nhưng không thể lưu file orders.dat";
		}
		cart.clear();
		cartScrollOffset = 0;
		return true;
	}
	return false;
}

void PosUI::draw(Vector2 uiMouse, Vector2 rawMouse, const Vector<Product>& products,
	const MenuLookup& menuLookup, const OrderQueue& orderQueue, double statusChangeTime) {

	Vector<int> filteredProductIndices = buildFilteredIndices(products);
	int totalPosRows = (filteredProductIndices.size() + 1) / 2;
	int maxPosRowScroll = maxVal(0, totalPosRows - 4);

	DrawUiText("Tạo Đơn Hàng Mới (POS)", 245, 104, 24, darkGreen);

	// Hiển thị trạng thái Hàng đợi FIFO (OrderQueue) và nút duyệt đơn trực tiếp
	if (!orderQueue.empty() && orderQueue.peek()) {
		Rectangle qBtn = {490, 105, 195, 28};
		Rectangle qv = Anim::Button(qBtn, amber, BrightenColor(amber, 18), uiMouse, true, 0.25f);
		String qLabel = String("Hàng đợi (") + toString(orderQueue.size()) + "): Duyệt #" + orderQueue.peek()->orderId;
		DrawUiTextInRect(qLabel.c_str(), qv, 11, RAYWHITE, 0, 6.0f);
	}

	String posCountInfo = String("Hiển thị ") + toString(filteredProductIndices.size()) + " món  •  Lăn chuột để cuộn";
	DrawUiTextRight(posCountInfo.c_str(), 915, 114, 12, muted);

	Rectangle searchBox = {245, 148, 285, 36};
	DrawUiInputBox(searchBox, search, "Tìm theo tên hoặc mã món...", searchFocused, rawMouse);

	const char* filterCategories[] = {"Tất cả", "Món chính", "Đồ uống", "Tráng miệng"};
	for (int c = 0; c < 4; ++c) {
		Rectangle catBtn = {540.0f + c * 88.0f, 148.0f, 82.0f, 36.0f};
		bool activeCat = categoryFilter == filterCategories[c];
		Color base = activeCat ? green : Color{235, 240, 234, 255};
		Color hover = activeCat ? BrightenColor(green, 15) : Color{220, 232, 222, 255};
		Rectangle v = Anim::Button(catBtn, base, hover, uiMouse, true, 0.25f, !activeCat);
		DrawUiTextInRect(filterCategories[c], v, 12, activeCat ? RAYWHITE : darkGreen, 0, 4.0f);
	}

	if (totalPosRows > 4) {
		Rectangle upBtn = {895, 148, 28, 17};
		Rectangle dnBtn = {895, 167, 28, 17};
		Anim::ArrowButton(upBtn, Anim::ArrowDir::Up, uiMouse, posScrollRowOffset > 0, darkGreen);
		Anim::ArrowButton(dnBtn, Anim::ArrowDir::Down, uiMouse, posScrollRowOffset < maxPosRowScroll, darkGreen);
	}

	for (int slot = 0; slot < 8; ++slot) {
		int filteredIdx = posScrollRowOffset * 2 + slot;
		if (filteredIdx >= filteredProductIndices.size()) break;
		const Product& product = products[filteredProductIndices[filteredIdx]];

		int column = slot % 2;
		int row = slot / 2;

		Rectangle card = {245.0f + column * 336.0f, 200.0f + row * 106.0f, 322, 94};
		bool cardHover = CheckCollisionPointRec(uiMouse, card);
		Rectangle cardV = Anim::Button(card, RAYWHITE, Color{252, 255, 253, 255}, uiMouse, true, 0.08f, true);
		DrawRectangleRoundedLines(cardV, 0.08f, 4, cardHover ? green : Color{225, 233, 226, 255});

		DrawRectangle(static_cast<int>(cardV.x), static_cast<int>(cardV.y + 10), 6, 74, product.color);

		DrawUiTextFitted(product.name.c_str(), cardV.x + 16, cardV.y + 12, 292, 15, {42, 55, 48, 255});
		DrawUiTextFitted((product.code + "  •  " + product.category).c_str(), cardV.x + 16, cardV.y + 36, 185, 12, {126, 136, 127, 255});
		DrawUiTextFitted(DataManager::formatCurrency(product.price).c_str(), cardV.x + 16, cardV.y + 60, 185, 14, green);

		Rectangle addBtn = {cardV.x + 208, cardV.y + 54, 100, 30};
		Rectangle addV = Anim::Button(addBtn, green, BrightenColor(green, 24), uiMouse, true, 0.25f);
		DrawUiTextInRect("+ Thêm", addV, 13, RAYWHITE, 0, 6.0f);
	}

	for (int i = cartFlyers.size() - 1; i >= 0; --i) {
		double age = GetTime() - cartFlyers[i].start;
		if (age > 0.45) {
			cartFlyers.eraseAt(i);
		}
	}
	for (int i = 0; i < cartFlyers.size(); ++i) {
		const CartFlyer& flyer = cartFlyers[i];
		double age = GetTime() - flyer.start;
		float progress = Anim::EaseOutCubic(static_cast<float>(age / 0.45));
		float cx = flyer.from.x + (flyer.to.x - flyer.from.x) * progress;
		float cy = flyer.from.y + (flyer.to.y - flyer.from.y) * progress - std::sin(progress * 3.14159f) * 45.0f;
		float radius = (1.0f - progress * 0.4f) * 9.0f;
		float alpha = 1.0f - progress * 0.3f;
		DrawCircleV({cx, cy}, radius, Anim::WithAlpha(flyer.color, alpha));
		DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), radius, Anim::WithAlpha(RAYWHITE, alpha));
	}

	DrawVerticalScrollbar({914, 200, 7, 412}, totalPosRows, 4, posScrollRowOffset);

	Rectangle cartPanel = {935, 104, 320, 602};
	DrawRectangleRounded(cartPanel, 0.04f, 4, RAYWHITE);
	DrawRectangleRoundedLines(cartPanel, 0.04f, 4, Color{220, 228, 221, 255});

	DrawUiText("ĐƠN HÀNG HIỆN TẠI", 952, 120, 15, darkGreen);
	if (!cart.empty()) {
		Rectangle clearBtn = {1175, 116, 66, 26};
		Rectangle clrV = Anim::Button(clearBtn, Color{245, 230, 230, 255}, Color{252, 215, 215, 255}, uiMouse, true, 0.2f);
		DrawUiTextInRect("Xoá giỏ", clrV, 11, Color{195, 65, 50, 255}, 0, 4.0f);
	}

	DrawLine(952, 150, 1238, 150, {220, 228, 221, 255});

	if (cart.empty()) {
		DrawUiTextCenterFitted("Giỏ hàng đang trống", 1095, 245, 280, 14, {140, 150, 142, 255});
		DrawUiTextCenterFitted("Nhấp chọn món bên trái để thêm", 1095, 270, 280, 12, {160, 170, 162, 255});
	} else {
		const int visCartRows = 7;
		for (int r = 0; r < visCartRows; ++r) {
			int idx = cartScrollOffset + r;
			if (idx >= cart.size()) break;
			const CartItem& item = cart[idx];
			float rowY = 162.0f + r * 34.0f;
			const MenuItem* mItem = menuLookup.find(item.productCode);
			const Product* p = findProduct(products, item.productCode);
			String dishName = mItem ? mItem->name : (p ? p->name : item.productCode);
			int unitPrice = mItem ? static_cast<int>(mItem->price) : (p ? p->price : 0);
			String lineName = toString(item.quantity) + "x " + dishName;

			if (item.productCode == cartFlashCode && (GetTime() - cartFlashTime < 0.45)) {
				float flashT = 1.0f - static_cast<float>((GetTime() - cartFlashTime) / 0.45);
				DrawRectangleRounded({944, rowY - 2, 290, 28}, 0.15f, 4,
					Anim::WithAlpha(Color{225, 248, 232, 255}, flashT * 0.95f));
			}

			DrawUiTextFitted(lineName.c_str(), 952, rowY + 3, 136, 13, {45, 56, 50, 255});

			int itemPrice = unitPrice * item.quantity;
			DrawUiTextRight(DataManager::formatCurrency(itemPrice).c_str(), 1178, rowY + 3, 12, green);

			Rectangle minusBtn = {1184, rowY, 22, 22};
			Rectangle mv = Anim::Button(minusBtn, Color{245, 235, 233, 255}, Color{250, 215, 212, 255}, uiMouse, true, 0.2f);
			DrawUiTextInRect("-", mv, 13, {180, 70, 50, 255}, 0, 2.0f);

			Rectangle plusBtn = {1210, rowY, 22, 22};
			Rectangle pv = Anim::Button(plusBtn, Color{234, 244, 238, 255}, Color{215, 240, 224, 255}, uiMouse, true, 0.2f);
			DrawUiTextInRect("+", pv, 13, green, 0, 2.0f);
		}
		DrawVerticalScrollbar({1240, 162, 6, 234}, cart.size(), visCartRows, cartScrollOffset);
	}

	int subtotal = cartTotal(cart, products, menuLookup);
	int discountAmount = computeDiscount(cart, products, menuLookup, discountPercent, comboPromotion);
	int finalTotal = maxVal(0, subtotal - discountAmount);

	DrawLine(952, 444, 1238, 444, {220, 228, 221, 255});
	DrawUiText("Tạm tính:", 952, 452, 13, {80, 92, 84, 255});
	DrawUiTextRight(DataManager::formatCurrency(subtotal).c_str(), 1238, 452, 13, {80, 92, 84, 255});

	const int presets[] = {0, 10, 20, 30, 50};
	for (int p = 0; p < 5; ++p) {
		Rectangle pBtn = {952.0f + p * 58.0f, 478.0f, 52.0f, 26.0f};
		bool active = discountPercent == presets[p];
		Color base = active ? amber : Color{238, 242, 238, 255};
		Color hover = active ? BrightenColor(amber, 18) : Color{225, 232, 225, 255};
		Rectangle pv = Anim::Button(pBtn, base, hover, uiMouse, true, 0.2f, !active);
		DrawUiTextInRect((toString(presets[p]) + "%").c_str(), pv, 12,
			active ? RAYWHITE : Color{45, 56, 50, 255}, 0, 2.0f);
	}

	DrawUiText("Giảm giá tuỳ chỉnh (%):", 952, 514, 12, {80, 92, 84, 255});
	Rectangle discBox = {1158, 510, 80, 26};
	DrawUiInputBox(discBox, discountInputText + " %", "0 %", discountInputFocused, rawMouse);

	Rectangle comboBtn = {952, 544, 286, 28};
	Color comboBase = comboPromotion ? amber : Color{245, 240, 230, 255};
	Color comboHover = comboPromotion ? BrightenColor(amber, 18) : Color{238, 230, 215, 255};
	Rectangle comboV = Anim::Button(comboBtn, comboBase, comboHover, uiMouse, true, 0.2f);
	DrawUiTextInRect(comboPromotion ? "✓ Ưu đãi Combo (-15.000 đ)" : "Bật ưu đãi Combo (-15.000 đ)",
		comboV, 12, comboPromotion ? RAYWHITE : Color{140, 95, 45, 255}, 0, 6.0f);

	DrawUiText("Đã giảm:", 952, 580, 12, amber);
	DrawUiTextRight((String("- ") + DataManager::formatCurrency(discountAmount)).c_str(), 1238, 580, 12, amber);

	DrawUiText("Tổng thanh toán:", 952, 604, 14, darkGreen);
	displayedTotal = Anim::Smooth(displayedTotal, static_cast<float>(finalTotal), 22.0f);
	DrawUiTextRight(DataManager::formatCurrency(static_cast<int>(std::round(displayedTotal))).c_str(), 1238, 601, 17, green);

	Rectangle payBtn = {952, 634, 286, 42};
	Rectangle payV = Anim::Button(payBtn, green, BrightenColor(green, 24), uiMouse, subtotal > 0, 0.2f);
	DrawUiTextInRect("THANH TOÁN ĐƠN HÀNG", payV, 14, RAYWHITE, 0, 8.0f);
	if (orderPlaced) {
		float pop = Anim::EaseOutBack(clampVal(static_cast<float>((GetTime() - statusChangeTime) / 0.35), 0.0f, 1.0f));
		rlPushMatrix();
		float pcx = payBtn.x + payBtn.width * 0.5f;
		float pcy = payBtn.y + 46.0f;
		rlTranslatef(pcx, pcy, 0.0f);
		rlScalef(pop, pop, 1.0f);
		rlTranslatef(-pcx, -pcy, 0.0f);
		DrawUiTextCenterFitted("Đã tạo đơn hàng thành công!", pcx, pcy, 280, 12, green);
		rlPopMatrix();
	}
}

