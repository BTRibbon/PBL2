#include "ManagementUI.h"
#include "UIFont.h"
#include "UIAnim.h"
#include "rlgl.h"

#include <cctype>
#include <cmath>
#include <cstdio>

namespace {
const Color darkGreen{35, 55, 53, 255};
const Color green{35, 105, 78, 255};
const Color muted{104, 115, 108, 255};
const Color panel{255, 255, 255, 255};
const Color panelAlt{248, 250, 248, 255};
const Color amber{180, 110, 40, 255};
const Color danger{195, 65, 50, 255};

static bool validDate(const String& value) {
	return value.size() == 10 && value[4] == '-' && value[7] == '-' &&
		std::isdigit(static_cast<unsigned char>(value[0])) &&
		std::isdigit(static_cast<unsigned char>(value[1])) &&
		std::isdigit(static_cast<unsigned char>(value[2])) &&
		std::isdigit(static_cast<unsigned char>(value[3])) &&
		std::isdigit(static_cast<unsigned char>(value[5])) &&
		std::isdigit(static_cast<unsigned char>(value[6])) &&
		std::isdigit(static_cast<unsigned char>(value[8])) &&
		std::isdigit(static_cast<unsigned char>(value[9]));
}

static bool validTime(const String& value) {
	if (value.size() != 5 || value[2] != ':') return false;
	if (!std::isdigit(static_cast<unsigned char>(value[0])) ||
		!std::isdigit(static_cast<unsigned char>(value[1])) ||
		!std::isdigit(static_cast<unsigned char>(value[3])) ||
		!std::isdigit(static_cast<unsigned char>(value[4]))) return false;
	int hours = value.substr(0, 2).toInt(99);
	int minutes = value.substr(3, 2).toInt(99);
	return hours < 24 && minutes < 60;
}
}

ManagementUI::ManagementUI(RestaurantChain& restaurantChain, Vector<Branch>& branchValues,
	ScheduleService& service, ShiftManager& manager, Vector<Timekeeping>& records,
	Vector<Employee*>& employeeUsers, Vector<Product>& productList, MenuLookup& lookup,
	Vector<IngredientType>& ingredients)
	: chain(restaurantChain), branches(branchValues), scheduleService(service),
	  shiftManager(manager), timekeepingRecords(records), employees(employeeUsers),
	  products(productList), menuLookup(lookup), ingredientTypes(ingredients) {}

Timekeeping* ManagementUI::getTimekeeping(Employee& currentUser) const {
	for (int i = 0; i < timekeepingRecords.size(); ++i) {
		if (timekeepingRecords[i].getEmployeeId() == currentUser.getId()) {
			return &timekeepingRecords[i];
		}
	}
	return nullptr;
}

void ManagementUI::drawButton(Rectangle bounds, const char* label, Vector2 mouse, bool enabled, Color bgColor) const {
	Rectangle v = Anim::Button(bounds, bgColor, mouse, enabled, 0.25f);
	DrawUiTextInRect(label, v, 13, RAYWHITE, 0, 6.0f);
}

void ManagementUI::drawPill(Rectangle bounds, const char* label, Vector2 mouse, bool selected, float fontSize) const {
	Color base = selected ? green : Color{232, 238, 233, 255};
	Color hover = selected ? BrightenColor(green, 14) : Color{212, 226, 216, 255};
	Rectangle v = Anim::Button(bounds, base, hover, mouse, true, 0.25f, !selected);
	DrawUiTextInRect(label, v, fontSize, selected ? RAYWHITE : darkGreen, 0, 6.0f);
}

float ManagementUI::beginDialogTransition(Rectangle dialog) const {
	float t = static_cast<float>((GetTime() - dialogOpenTime) / 0.28);
	float e = Anim::EaseOutBack(t);
	float fade = Anim::EaseOutCubic(t);
	DrawRectangle(0, 0, 1280, 760, Color{0, 0, 0, static_cast<unsigned char>(140.0f * fade)});
	float s = 0.9f + 0.1f * e;
	float cx = dialog.x + dialog.width * 0.5f;
	float cy = dialog.y + dialog.height * 0.5f;
	rlPushMatrix();
	rlTranslatef(cx, cy + (1.0f - fade) * 18.0f, 0.0f);
	rlScalef(s, s, 1.0f);
	rlTranslatef(-cx, -cy, 0.0f);
	DrawRectangleRounded({dialog.x + 2, dialog.y + 10, dialog.width, dialog.height}, 0.05f, 8, Color{0, 0, 0, static_cast<unsigned char>(50.0f * fade)});
	return fade;
}

void ManagementUI::endDialogTransition() const {
	rlPopMatrix();
}

void ManagementUI::drawManagementHeader(const char* title, const char* subtitle) const {
	String fullTitle = String(title) + " - " + chain.getName();
	DrawUiTextFitted(fullTitle.c_str(), 245, 104, 1000, 24, darkGreen);
	DrawUiTextFitted(subtitle, 245, 136, 1000, 13, muted);
}

void ManagementUI::drawMenuTab(Vector2 mouse, Employee& currentUser) {
	bool canEditMenu = currentUser.canManageAllBranches();

	const int visibleRows = 8;
	int total = products.size();
	int maxScroll = maxVal(0, total - visibleRows);
	menuScrollOffset = clampVal(menuScrollOffset, 0, maxScroll);

	int firstIdx = total == 0 ? 0 : menuScrollOffset + 1;
	int lastIdx = minVal(total, menuScrollOffset + visibleRows);
	String countText = String("DANH SÁCH MÓN ĂN (") + toString(firstIdx) + "-" +
		toString(lastIdx) + " / " + toString(total) + " món  •  Bảng băm MenuLookup: " +
		toString(menuLookup.size()) + " mục)";
	DrawUiTextFitted(countText.c_str(), 245, 200, 680, 13, darkGreen);

	if (total > visibleRows) {
		Anim::ArrowButton({945, 192, 36, 32}, Anim::ArrowDir::Up, mouse, menuScrollOffset > 0, darkGreen);
		Anim::ArrowButton({987, 192, 36, 32}, Anim::ArrowDir::Down, mouse, menuScrollOffset < maxScroll, darkGreen);
	}

	if (canEditMenu) {
		drawButton({1035, 192, 210, 32}, "+ THÊM MÓN MỚI", mouse, true, green);
	}

	int tableY = 232;
	DrawRectangle(245, tableY, 985, 34, Color{232, 238, 233, 255});
	DrawUiText("MÃ MÓN", 265, tableY + 9, 12, darkGreen);
	DrawUiText("TÊN MÓN ĂN", 375, tableY + 9, 12, darkGreen);
	DrawUiText("PHÂN LOẠI", 685, tableY + 9, 12, darkGreen);
	DrawUiText("ĐƠN GIÁ", 855, tableY + 9, 12, darkGreen);
	DrawUiText("THAO TÁC", 1040, tableY + 9, 12, darkGreen);

	int rowY = tableY + 38;
	for (int r = 0; r < visibleRows; ++r) {
		int i = menuScrollOffset + r;
		if (i >= total) break;

		Rectangle rowRec = {245, static_cast<float>(rowY), 985, 46};
		bool rowHover = CheckCollisionPointRec(mouse, rowRec) && !isDialogOpen();
		DrawRectangleRec(rowRec, rowHover ? Color{240, 247, 242, 255} : ((i % 2 == 0) ? panel : panelAlt));
		DrawRectangleLines(245, rowY, 985, 46, Color{232, 236, 232, 255});

		Rectangle codeBadge = {262, static_cast<float>(rowY + 10), 82, 26};
		DrawRectangleRounded(codeBadge, 0.2f, 4, Color{238, 243, 240, 255});
		DrawUiTextInRect(products[i].code.c_str(), codeBadge, 13, darkGreen, 0, 4.0f);

		DrawUiTextFitted(products[i].name.c_str(), 375, rowY + 15, 290, 14, Color{45, 56, 50, 255});

		Rectangle catBadge = {680, static_cast<float>(rowY + 10), 125, 26};
		DrawRectangleRounded(catBadge, 0.25f, 4, Color{238, 243, 240, 255});
		DrawUiTextInRect(products[i].category.c_str(), catBadge, 12, Color{60, 95, 75, 255}, 0, 6.0f);

		DrawUiTextFitted(DataManager::formatCurrency(products[i].price).c_str(), 855, rowY + 15, 150, 14, green);

		if (canEditMenu) {
			drawButton({1025, static_cast<float>(rowY + 8), 86, 30}, "Sửa", mouse, true, amber);
			drawButton({1121, static_cast<float>(rowY + 8), 86, 30}, "Xoá", mouse, true, danger);
		} else {
			DrawUiText("Chỉ xem", 1045, rowY + 15, 13, muted);
		}

		rowY += 48;
	}

	DrawVerticalScrollbar({1236, static_cast<float>(tableY + 38), 8, static_cast<float>(visibleRows * 48 - 2)},
		total, visibleRows, menuScrollOffset);

	if (!canEditMenu) {
		DrawUiTextFitted("Lưu ý: Chỉ Giám đốc chuỗi (ChainManager) mới có quyền thêm, sửa, xoá món ăn.",
			245, 665, 980, 13, amber);
	}
}

void ManagementUI::drawMenuDialog(Vector2 mouse) {
	if (!isMenuDialogOpen) return;

	Rectangle dialog = {380, 135, 520, 460};
	beginDialogTransition(dialog);
	DrawRectangleRounded(dialog, 0.05f, 8, panel);
	DrawRectangleRoundedLines(dialog, 0.05f, 8, green);

	const char* dialogTitle = menuDialogMode == 0 ? "THÊM MÓN ĂN MỚI (GIÁM ĐỐC CHUỖI)" : "CHỈNH SỬA MÓN ĂN";
	DrawUiTextFitted(dialogTitle, dialog.x + 30, dialog.y + 22, 460, 19, darkGreen);
	DrawUiTextFitted("Điền thông tin bên dưới rồi nhấn Lưu (hoặc phím Enter)", dialog.x + 30, dialog.y + 50, 460, 12, muted);

	DrawUiText("Mã món (Ví dụ: M113):", dialog.x + 30, dialog.y + 84, 13, darkGreen);
	Rectangle codeBox = {dialog.x + 30, dialog.y + 108, 460, 38};
	DrawUiInputBox(codeBox, menuCodeInput, "Nhập mã món...", menuDialogField == 0, mouse);

	DrawUiText("Tên món ăn:", dialog.x + 30, dialog.y + 158, 13, darkGreen);
	Rectangle nameBox = {dialog.x + 30, dialog.y + 182, 460, 38};
	DrawUiInputBox(nameBox, menuNameInput, "Nhập tên món ăn...", menuDialogField == 1, mouse);

	DrawUiText("Phân loại:", dialog.x + 30, dialog.y + 232, 13, darkGreen);
	const char* categories[] = {"Món chính", "Đồ uống", "Tráng miệng"};
	for (int c = 0; c < 3; ++c) {
		Rectangle catPill = {dialog.x + 30 + c * 156.0f, dialog.y + 256, 146, 34};
		drawPill(catPill, categories[c], mouse, menuCategoryInput == categories[c]);
	}

	DrawUiText("Đơn giá (VNĐ):", dialog.x + 30, dialog.y + 304, 13, darkGreen);
	Rectangle priceBox = {dialog.x + 30, dialog.y + 328, 460, 38};
	DrawUiInputBox(priceBox, menuPriceInput, "Nhập giá bán (VD: 45000)...", menuDialogField == 3, mouse);

	DrawUiTextFitted("Phím Tab: Chuyển ô  •  Enter: Lưu  •  Esc: Đóng", dialog.x + 30, dialog.y + 375, 460, 12, muted);

	drawButton({dialog.x + 30, dialog.y + 402, 220, 40}, "LƯU MÓN ĂN", mouse, true, green);
	drawButton({dialog.x + 270, dialog.y + 402, 220, 40}, "HỦY BỎ", mouse, true, Color{150, 160, 152, 255});
	endDialogTransition();
}

void ManagementUI::drawIngredientDialog(Vector2 mouse) {
	if (!isIngredientDialogOpen) return;

	Rectangle dialog = {380, 135, 520, 460};
	beginDialogTransition(dialog);
	DrawRectangleRounded(dialog, 0.05f, 8, panel);
	DrawRectangleRoundedLines(dialog, 0.05f, 8, green);

	DrawUiTextFitted("THÊM LOẠI NGUYÊN LIỆU MỚI", dialog.x + 30, dialog.y + 22, 460, 19, darkGreen);
	DrawUiTextFitted("Nguyên liệu mới sẽ được áp dụng cho toàn bộ chuỗi chi nhánh", dialog.x + 30, dialog.y + 50, 460, 12, muted);

	DrawUiText("Mã nguyên liệu (VD: SUGAR, TEA):", dialog.x + 30, dialog.y + 84, 13, darkGreen);
	Rectangle idBox = {dialog.x + 30, dialog.y + 108, 460, 38};
	DrawUiInputBox(idBox, ingIdInput, "Nhập mã nguyên liệu viết hoa...", ingDialogField == 0, mouse);

	DrawUiText("Tên nguyên liệu:", dialog.x + 30, dialog.y + 158, 13, darkGreen);
	Rectangle nameBox = {dialog.x + 30, dialog.y + 182, 460, 38};
	DrawUiInputBox(nameBox, ingNameInput, "Nhập tên nguyên liệu (VD: Đường cát)...", ingDialogField == 1, mouse);

	DrawUiText("Đơn vị tính:", dialog.x + 30, dialog.y + 232, 13, darkGreen);
	const char* units[] = {"kg", "lít", "hộp", "gói"};
	for (int u = 0; u < 4; ++u) {
		Rectangle uPill = {dialog.x + 30 + u * 116.0f, dialog.y + 256, 108, 34};
		drawPill(uPill, units[u], mouse, ingUnitInput == units[u]);
	}

	DrawUiText("Số lượng tồn khởi tạo cho mỗi chi nhánh:", dialog.x + 30, dialog.y + 304, 13, darkGreen);
	Rectangle stockBox = {dialog.x + 30, dialog.y + 328, 460, 38};
	DrawUiInputBox(stockBox, ingInitialStockInput, "Nhập số lượng ban đầu (VD: 50)...", ingDialogField == 3, mouse);

	DrawUiTextFitted("Phím Tab: Chuyển ô  •  Enter: Lưu  •  Esc: Đóng", dialog.x + 30, dialog.y + 375, 460, 12, muted);

	drawButton({dialog.x + 30, dialog.y + 402, 220, 40}, "THÊM NGUYÊN LIỆU", mouse, true, green);
	drawButton({dialog.x + 270, dialog.y + 402, 220, 40}, "HỦY BỎ", mouse, true, Color{150, 160, 152, 255});
	endDialogTransition();
}

void ManagementUI::drawInventoryTab(Vector2 mouse, Employee& currentUser) {
	drawManagementHeader("Quản lý Kho & Thực đơn", "Quản lý thực đơn toàn chuỗi và tồn kho nguyên liệu từng chi nhánh");

	Rectangle tabMenuRec = {245, 154, 220, 34};
	Rectangle tabStockRec = {475, 154, 240, 34};
	drawPill(tabMenuRec, "THỰC ĐƠN MÓN ĂN", mouse, managementSubTab == 0);
	drawPill(tabStockRec, "KHO NGUYÊN LIỆU", mouse, managementSubTab == 1);
	{
		Rectangle target = managementSubTab == 0 ? tabMenuRec : tabStockRec;
		static float indX = -1.0f, indW = 0.0f;
		if (indX < 0.0f) { indX = target.x; indW = target.width; }
		indX = Anim::Smooth(indX, target.x + 20.0f, 16.0f);
		indW = Anim::Smooth(indW, target.width - 40.0f, 16.0f);
		DrawRectangleRounded({indX, target.y + target.height + 3.0f, indW, 3.0f}, 1.0f, 4, Color{248, 218, 154, 255});
	}

	if (managementSubTab == 0) {
		drawMenuTab(mouse, currentUser);
		return;
	}

	bool isChainMgr = currentUser.canManageAllBranches();
	bool canAdjustStock = currentUser.canManageBranches();

	if (isChainMgr) {
		drawButton({1015, 154, 230, 34}, "+ THÊM LOẠI NGUYÊN LIỆU", mouse, true, green);
	}

	if (!isChainMgr) {
		for (int i = 0; i < branches.size(); ++i) {
			if (branches[i].getId() == currentUser.getBranchId()) {
				selectedInventoryBranch = i;
				break;
			}
		}
	} else {
		selectedInventoryBranch = clampVal(selectedInventoryBranch, 0, maxVal(0, branches.size() - 1));
	}

	int branchCardY = 198;
	if (isChainMgr && !branches.empty()) {
		DrawUiText("Chi nhánh:", 245, 203, 13, darkGreen);
		const int maxVisBranches = 4;
		int totalBr = branches.size();
		invBranchScrollOffset = clampVal(invBranchScrollOffset, 0, maxVal(0, totalBr - maxVisBranches));

		float startX = 325.0f;
		if (totalBr > maxVisBranches) {
			Anim::ArrowButton({325, 198, 28, 28}, Anim::ArrowDir::Left, mouse, invBranchScrollOffset > 0, darkGreen);
			startX = 360.0f;
		}
		int shown = minVal(totalBr - invBranchScrollOffset, maxVisBranches);
		for (int b = 0; b < shown; ++b) {
			int bIdx = invBranchScrollOffset + b;
			Rectangle bPill = {startX + b * 204.0f, 198.0f, 196.0f, 28.0f};
			drawPill(bPill, branches[bIdx].getName().c_str(), mouse, selectedInventoryBranch == bIdx, 12.0f);
		}
		if (totalBr > maxVisBranches) {
			Anim::ArrowButton({startX + shown * 204.0f, 198, 28, 28}, Anim::ArrowDir::Right, mouse, invBranchScrollOffset < totalBr - maxVisBranches, darkGreen);
		}
		branchCardY = 234;
	}

	if (branches.empty()) return;
	Branch& activeBranch = branches[selectedInventoryBranch];

	Rectangle bInfoCard = {245, static_cast<float>(branchCardY), 1000, 52};
	DrawRectangleRounded(bInfoCard, 0.1f, 4, panel);
	DrawRectangleRoundedLines(bInfoCard, 0.1f, 4, Color{220, 228, 221, 255});

	String bHeader = activeBranch.getName() + " (" + activeBranch.getId() + ")  •  Giờ mở cửa: " +
		activeBranch.getOpeningHours().getOpenTime() + " - " + activeBranch.getOpeningHours().getCloseTime() +
		"  •  Số loại nguyên liệu: " + toString(ingredientTypes.size());
	DrawUiTextFitted(bHeader.c_str(), 265, branchCardY + 17, 960, 14, darkGreen);

	int tableHeaderY = branchCardY + 62;
	int visibleIngRows = isChainMgr ? 6 : 7;
	int totalIng = ingredientTypes.size();
	int maxIngScroll = maxVal(0, totalIng - visibleIngRows);
	ingredientScrollOffset = clampVal(ingredientScrollOffset, 0, maxIngScroll);

	DrawRectangle(245, tableHeaderY, 985, 34, Color{232, 238, 233, 255});
	DrawUiText("MÃ NL", 265, tableHeaderY + 9, 12, darkGreen);
	DrawUiText("TÊN NGUYÊN LIỆU", 380, tableHeaderY + 9, 12, darkGreen);
	DrawUiText("ĐƠN VỊ", 630, tableHeaderY + 9, 12, darkGreen);
	DrawUiText("TỒN KHO HIỆN TẠI", 740, tableHeaderY + 9, 12, darkGreen);
	DrawUiText("NHẬP / XUẤT KHO", 925, tableHeaderY + 9, 12, darkGreen);
	if (isChainMgr) {
		DrawUiText("XOÁ LOẠI", 1135, tableHeaderY + 9, 12, darkGreen);
	}

	int rowY = tableHeaderY + 38;
	for (int r = 0; r < visibleIngRows; ++r) {
		int idx = ingredientScrollOffset + r;
		if (idx >= totalIng) break;
		const IngredientType& ing = ingredientTypes[idx];
		int currentQty = activeBranch.getStock(ing.id);

		Rectangle rowRec = {245, static_cast<float>(rowY), 985, 46};
		DrawRectangleRec(rowRec, (idx % 2 == 0) ? panel : panelAlt);
		DrawRectangleLines(245, rowY, 985, 46, Color{232, 236, 232, 255});

		Rectangle idBadge = {262, static_cast<float>(rowY + 10), 95, 26};
		DrawRectangleRounded(idBadge, 0.2f, 4, Color{238, 243, 240, 255});
		DrawUiTextInRect(ing.id.c_str(), idBadge, 12, darkGreen, 0, 4.0f);

		DrawUiTextFitted(ing.name.c_str(), 380, rowY + 15, 235, 14, Color{45, 56, 50, 255});
		DrawUiTextFitted(ing.unit.c_str(), 630, rowY + 15, 90, 13, muted);

		String stockStr = toString(currentQty) + " " + ing.unit;
		DrawUiTextFitted(stockStr.c_str(), 740, rowY + 15, 160, 14, currentQty > 10 ? green : amber);

		if (canAdjustStock) {
			drawButton({915, static_cast<float>(rowY + 8), 90, 30}, "+10", mouse, true, green);
			drawButton({1015, static_cast<float>(rowY + 8), 90, 30}, "-5", mouse, currentQty >= 5, amber);
		} else {
			DrawUiText("Chỉ xem", 950, rowY + 15, 13, muted);
		}

		if (isChainMgr) {
			drawButton({1125, static_cast<float>(rowY + 8), 88, 30}, "Xoá loại", mouse, totalIng > 1, danger);
		}

		rowY += 48;
	}

	DrawVerticalScrollbar({1236, static_cast<float>(tableHeaderY + 38), 8, static_cast<float>(visibleIngRows * 48 - 2)},
		totalIng, visibleIngRows, ingredientScrollOffset);

	if (!isChainMgr) {
		DrawUiTextFitted("Lưu ý: Quản lý chi nhánh được phép nhập/xuất tồn kho. Chỉ Giám đốc chuỗi được thêm/xoá loại nguyên liệu.",
			245, 672, 980, 12, muted);
	}
}

void ManagementUI::drawScheduleTab(Vector2 mouse, Employee& currentUser) {
	drawManagementHeader("Lịch làm việc & Phân ca", "Tạo ca làm việc mở, đăng ký ca trực và kiểm tra độ phủ nhân sự theo chi nhánh");

	bool isChainMgr = currentUser.canManageAllBranches();
	bool canManage = currentUser.canManageBranches();

	drawButton({245, 166, 140, 36}, "+ TẠO CA LÀM", mouse, canManage, green);

	Rectangle dBox = {395, 166, 148, 36};
	Rectangle sBox = {551, 166, 118, 36};
	Rectangle eBox = {677, 166, 118, 36};
	DrawUiInputBox(dBox, String("Ngày: ") + dateInput, "YYYY-MM-DD", focusedField == 0, mouse);
	DrawUiInputBox(sBox, String("Vào: ") + startInput, "HH:MM", focusedField == 1, mouse);
	DrawUiInputBox(eBox, String("Ra: ") + endInput, "HH:MM", focusedField == 2, mouse);

	if (isChainMgr && !branches.empty()) {
		scheduleBranchIndex = clampVal(scheduleBranchIndex, 0, branches.size() - 1);
		String brLabel = String("CN: ") + branches[scheduleBranchIndex].getId();
		drawButton({803, 166, 105, 36}, brLabel.c_str(), mouse, true, amber);
		drawButton({916, 166, 329, 36}, "KIỂM TRA ĐỘ PHỦ TẤT CẢ CN", mouse, true, darkGreen);
	} else {
		String checkLabel = String("KIỂM TRA ĐỘ PHỦ (") + currentUser.getBranchId() + ")";
		drawButton({805, 166, 260, 36}, checkLabel.c_str(), mouse, canManage, darkGreen);
	}

	DrawUiTextFitted("Nhấp vào ô Ngày / Vào / Ra để nhập. Quản lý kiểm tra độ phủ cho chi nhánh mình; Giám đốc chuỗi kiểm tra toàn bộ.",
		245, 210, 1000, 12, muted);

	Vector<Schedule*> visibleSchedules;
	const Vector<Schedule*>& allSchedules = scheduleService.getSchedules();
	for (int i = 0; i < allSchedules.size(); ++i) {
		Schedule* schedule = allSchedules[i];
		if (isChainMgr || schedule->getBranchId() == "ALL" || schedule->getBranchId() == currentUser.getBranchId()) {
			visibleSchedules.push_back(schedule);
		}
	}

	const int maxVisibleShifts = 5;
	int totalShifts = visibleSchedules.size();
	int maxShiftScroll = maxVal(0, totalShifts - maxVisibleShifts);
	scheduleScrollOffset = clampVal(scheduleScrollOffset, 0, maxShiftScroll);

	String listTitle = String("DANH SÁCH CA LÀM VIỆC (") + toString(totalShifts) + " ca)";
	DrawUiTextFitted(listTitle.c_str(), 245, 236, 600, 14, darkGreen);

	int y = 262;
	for (int r = 0; r < maxVisibleShifts; ++r) {
		int idx = scheduleScrollOffset + r;
		if (idx >= totalShifts) break;
		const Schedule* schedule = visibleSchedules[idx];

		DrawRectangleRounded({245, static_cast<float>(y), 630, 72}, 0.08f, 4, panel);
		DrawRectangleRoundedLines({245, static_cast<float>(y), 630, 72}, 0.08f, 4, Color{225, 233, 226, 255});

		char details[200];
		std::snprintf(details, sizeof(details), "Mã ca: %s  |  Ngày: %s  |  %s - %s  |  CN: %s",
			schedule->getId().c_str(), schedule->getDate().c_str(),
			schedule->getStartTime().c_str(), schedule->getEndTime().c_str(),
			schedule->getBranchId().c_str());
		DrawUiTextFitted(details, 262, y + 14, 380, 13, darkGreen);

		bool covered = schedule->hasCoverage();
		std::snprintf(details, sizeof(details), "Nhân sự: %d/%d  •  %s",
			schedule->getAssignedEmployeeCount(), schedule->getRequiredEmployees(),
			covered ? "Đã đủ người" : "Đang thiếu người");
		DrawUiTextFitted(details, 262, y + 42, 380, 13, covered ? green : amber);

		if (schedule->isOpenShift())
			drawButton({655, static_cast<float>(y + 20), 96, 32}, "Đăng ký", mouse, true, green);
		if (canManage)
			drawButton({761, static_cast<float>(y + 20), 96, 32}, "Xoá ca", mouse, true, danger);

		y += 82;
	}

	DrawVerticalScrollbar({880, 262, 7, static_cast<float>(maxVisibleShifts * 82 - 10)},
		totalShifts, maxVisibleShifts, scheduleScrollOffset);

	Rectangle rightPanel = {900, 262, 345, 412};
	DrawRectangleRounded(rightPanel, 0.06f, 4, panel);
	DrawRectangleRoundedLines(rightPanel, 0.06f, 4, Color{225, 233, 226, 255});

	String panelScope = isChainMgr ? String("CẢNH BÁO CA (TOÀN CHUỖI)") : (String("CẢNH BÁO CA (") + currentUser.getBranchId() + ")");
	DrawUiTextFitted(panelScope.c_str(), rightPanel.x + 18, rightPanel.y + 16, 310, 14, darkGreen);
	DrawLine(static_cast<int>(rightPanel.x + 18), static_cast<int>(rightPanel.y + 42),
		static_cast<int>(rightPanel.x + rightPanel.width - 18), static_cast<int>(rightPanel.y + 42),
		Color{230, 236, 231, 255});

	bool minSatisfied = scheduleService.validateMinimumShifts(currentUser, "2026-01-01", "2026-12-31");
	Rectangle statusBadge = {rightPanel.x + 18, rightPanel.y + 54, rightPanel.width - 36, 36};
	DrawRectangleRounded(statusBadge, 0.2f, 4, minSatisfied ? Color{232, 245, 238, 255} : Color{252, 242, 230, 255});
	DrawUiTextInRect(minSatisfied ? "Chỉ tiêu ca tối thiểu: ĐẠT" : "Chỉ tiêu ca tối thiểu: CHƯA ĐẠT",
		statusBadge, 13, minSatisfied ? green : amber, 0, 8.0f);

	DrawUiText("Kết quả kiểm tra độ phủ ca:", rightPanel.x + 18, rightPanel.y + 104, 13, muted);
	float alertY = rightPanel.y + 130;
	const Vector<String>& alerts = shiftManager.getAlerts();
	if (alerts.empty()) {
		DrawUiTextWrapped("Tất cả các ca trong phạm vi kiểm tra đã đủ nhân sự hoặc chưa có cảnh báo mới.",
			{rightPanel.x + 18, alertY, rightPanel.width - 36, 60}, 12, green);
	} else {
		for (int i = 0; i < alerts.size(); ++i) {
			if (alertY + 34 > rightPanel.y + rightPanel.height - 12) break;
			DrawUiTextWrapped(alerts[i].c_str(), {rightPanel.x + 18, alertY, rightPanel.width - 36, 36}, 12, amber);
			alertY += 38;
		}
	}
}

void ManagementUI::drawAttendanceTab(Vector2 mouse, Employee& currentUser) {
	drawManagementHeader("Điểm danh & Chấm công", "Ghi nhận giờ vào ca và ra ca theo thời gian hệ thống thực tế");

	drawButton({245, 172, 185, 40}, "VÀO CA (CHECK-IN)", mouse, true, green);
	drawButton({445, 172, 185, 40}, "RA CA (CHECK-OUT)", mouse, true, amber);

	Timekeeping* records = getTimekeeping(currentUser);
	String empInfo = String("Nhân sự: ") + currentUser.getName() + "  •  Mã NV: " +
		(records ? records->getEmployeeId() : currentUser.getId());
	DrawUiTextFitted(empInfo.c_str(), 245, 230, 980, 15, darkGreen);

	int totalRecs = records ? records->getRecords().size() : 0;
	const int visibleRows = 8;
	int maxScroll = maxVal(0, totalRecs - visibleRows);
	attendanceScrollOffset = clampVal(attendanceScrollOffset, 0, maxScroll);

	DrawUiText((String("LỊCH SỬ ĐIỂM DANH (") + toString(totalRecs) + " lượt)").c_str(), 245, 264, 13, muted);

	int tableY = 290;
	DrawRectangle(245, tableY, 985, 34, Color{232, 238, 233, 255});
	DrawUiText("MÃ CA TRỰC", 265, tableY + 9, 12, darkGreen);
	DrawUiText("THỜI GIAN VÀO CA", 430, tableY + 9, 12, darkGreen);
	DrawUiText("THỜI GIAN RA CA", 690, tableY + 9, 12, darkGreen);
	DrawUiText("TRẠNG THÁI", 960, tableY + 9, 12, darkGreen);

	int y = tableY + 38;
	if (records) {
		const Vector<AttendanceRecord>& recList = records->getRecords();
		for (int r = 0; r < visibleRows; ++r) {
			int idx = attendanceScrollOffset + r;
			if (idx >= totalRecs) break;
			const AttendanceRecord& record = recList[idx];

			DrawRectangle(245, y, 985, 40, (idx % 2 == 0) ? panel : panelAlt);
			DrawRectangleLines(245, y, 985, 40, Color{225, 233, 226, 255});

			DrawUiTextFitted(record.scheduleId.c_str(), 265, y + 11, 145, 13, darkGreen);
			DrawUiTextFitted(record.checkIn.empty() ? "---" : record.checkIn.c_str(), 430, y + 11, 240, 13, darkGreen);
			DrawUiTextFitted(record.checkOut.empty() ? "Đang trong ca..." : record.checkOut.c_str(), 690, y + 11, 240, 13,
				record.checkOut.empty() ? amber : darkGreen);
			DrawUiTextFitted(record.isPresent ? "Có mặt đầy đủ" : "Chưa hoàn tất", 960, y + 11, 250, 13,
				record.isPresent ? green : amber);
			y += 44;
		}
	}

	DrawVerticalScrollbar({1236, static_cast<float>(tableY + 38), 8, static_cast<float>(visibleRows * 44 - 4)},
		totalRecs, visibleRows, attendanceScrollOffset);
}

bool ManagementUI::submitMenuDialog() {
	if (menuCodeInput.empty() || menuNameInput.empty() || menuPriceInput.empty()) {
		message = "Vui lòng nhập đầy đủ Mã món, Tên món và Đơn giá!";
		return true;
	}
	int price = 0;
	if (!menuPriceInput.tryParseInt(price)) {
		message = "Giá tiền không hợp lệ. Vui lòng nhập số nguyên!";
		return true;
	}
	if (price <= 0) {
		message = "Giá tiền phải lớn hơn 0!";
		return true;
	}

	Color col = {235, 142, 72, 255};
	if (menuCategoryInput == "Món chính") col = {210, 92, 68, 255};
	else if (menuCategoryInput == "Đồ uống") col = {82, 150, 128, 255};
	else if (menuCategoryInput == "Tráng miệng") col = {192, 122, 155, 255};

	if (menuDialogMode == 0) {
		for (int i = 0; i < products.size(); ++i) {
			if (products[i].code == menuCodeInput) {
				message = String("Mã món ") + menuCodeInput + " đã tồn tại! Vui lòng chọn mã khác.";
				return true;
			}
		}
		Product newProd(menuCodeInput, menuNameInput, menuCategoryInput, price, col);
		products.push_back(newProd);
		menuLookup.addItem(MenuItem(menuCodeInput, menuNameInput, static_cast<float>(price)));
		menuScrollOffset = maxVal(0, products.size() - 8);
		message = String("Đã thêm món mới thành công: ") + menuNameInput + " (" + menuCodeInput + ")";
	} else if (menuDialogMode == 1 && editingProductIndex >= 0 && editingProductIndex < products.size()) {
		String oldCode = products[editingProductIndex].code;
		if (oldCode != menuCodeInput) {
			menuLookup.removeItem(oldCode);
		}
		products[editingProductIndex].code = menuCodeInput;
		products[editingProductIndex].name = menuNameInput;
		products[editingProductIndex].category = menuCategoryInput;
		products[editingProductIndex].price = price;
		products[editingProductIndex].color = col;
		menuLookup.addItem(MenuItem(menuCodeInput, menuNameInput, static_cast<float>(price)));
		message = String("Đã cập nhật món ăn: ") + menuNameInput;
	}
	isMenuDialogOpen = false;
	return true;
}

bool ManagementUI::submitIngredientDialog() {
	if (ingIdInput.empty() || ingNameInput.empty() || ingUnitInput.empty()) {
		message = "Vui lòng nhập đầy đủ Mã và Tên nguyên liệu!";
		return true;
	}
	ingIdInput = ingIdInput.toUpper();
	for (int i = 0; i < ingredientTypes.size(); ++i) {
		if (ingredientTypes[i].id == ingIdInput) {
			message = String("Mã nguyên liệu ") + ingIdInput + " đã tồn tại!";
			return true;
		}
	}
	int initStock = maxVal(0, ingInitialStockInput.toInt(0));

	ingredientTypes.push_back(IngredientType(ingIdInput, ingNameInput, ingUnitInput));
	for (int i = 0; i < branches.size(); ++i) {
		if (initStock > 0) branches[i].addStock(ingIdInput, initStock);
	}
	ingredientScrollOffset = maxVal(0, ingredientTypes.size() - 6);
	isIngredientDialogOpen = false;
	message = String("Đã thêm loại nguyên liệu mới: ") + ingNameInput + " (" + ingIdInput + ")";
	return true;
}

void ManagementUI::updateScroll(Vector2 mouse, float wheelMove, int activeTab, Employee& currentUser) {
	if (std::fabs(wheelMove) < 0.01f || isDialogOpen()) return;
	int step = (wheelMove > 0) ? -1 : 1;

	if (activeTab == 3) {
		if (managementSubTab == 0) {
			int maxScroll = maxVal(0, products.size() - 8);
			menuScrollOffset = clampVal(menuScrollOffset + step, 0, maxScroll);
		} else {
			bool isChainMgr = currentUser.canManageAllBranches();
			if (isChainMgr && CheckCollisionPointRec(mouse, {245, 194, 1000, 36})) {
				int maxBrScroll = maxVal(0, branches.size() - 4);
				invBranchScrollOffset = clampVal(invBranchScrollOffset + step, 0, maxBrScroll);
			} else {
				int visRows = isChainMgr ? 6 : 7;
				int maxIngScroll = maxVal(0, ingredientTypes.size() - visRows);
				ingredientScrollOffset = clampVal(ingredientScrollOffset + step, 0, maxIngScroll);
			}
		}
	} else if (activeTab == 4) {
		int maxScroll = maxVal(0, scheduleService.getSchedules().size() - 5);
		scheduleScrollOffset = clampVal(scheduleScrollOffset + step, 0, maxScroll);
	} else if (activeTab == 5) {
		Timekeeping* records = getTimekeeping(currentUser);
		int total = records ? records->getRecords().size() : 0;
		int maxScroll = maxVal(0, total - 8);
		attendanceScrollOffset = clampVal(attendanceScrollOffset + step, 0, maxScroll);
	}
}

bool ManagementUI::updateInput(int key, int charPressed) {
	if (isMenuDialogOpen) {
		if (key == KEY_ESCAPE) {
			isMenuDialogOpen = false;
			return false;
		}
		if (key == KEY_ENTER) {
			return submitMenuDialog();
		}
		if (key == KEY_TAB) {
			menuDialogField = (menuDialogField == 0) ? 1 : (menuDialogField == 1 ? 3 : 0);
			return false;
		}

		String* target = nullptr;
		if (menuDialogField == 0) target = &menuCodeInput;
		else if (menuDialogField == 1) target = &menuNameInput;
		else if (menuDialogField == 3) target = &menuPriceInput;

		if (key == KEY_BACKSPACE && target && !target->empty()) {
			PopUtf8Char(*target);
			return false;
		}
		if (charPressed >= 32 && target) {
			if (menuDialogField == 3) {
				if (std::isdigit(charPressed) && target->size() < 10) {
					target->push_back(static_cast<char>(charPressed));
				}
			} else {
				AppendUtf8Codepoint(*target, charPressed, 36);
			}
		}
		return false;
	}

	if (isIngredientDialogOpen) {
		if (key == KEY_ESCAPE) {
			isIngredientDialogOpen = false;
			return false;
		}
		if (key == KEY_ENTER) {
			return submitIngredientDialog();
		}
		if (key == KEY_TAB) {
			ingDialogField = (ingDialogField == 0) ? 1 : (ingDialogField == 1 ? 3 : 0);
			return false;
		}

		String* target = nullptr;
		if (ingDialogField == 0) target = &ingIdInput;
		else if (ingDialogField == 1) target = &ingNameInput;
		else if (ingDialogField == 3) target = &ingInitialStockInput;

		if (key == KEY_BACKSPACE && target && !target->empty()) {
			PopUtf8Char(*target);
			return false;
		}
		if (charPressed >= 32 && target) {
			if (ingDialogField == 3) {
				if (std::isdigit(charPressed) && target->size() < 8) {
					target->push_back(static_cast<char>(charPressed));
				}
			} else if (ingDialogField == 0) {
				if (std::isalnum(charPressed) && target->size() < 14) {
					target->push_back(static_cast<char>(std::toupper(charPressed)));
				}
			} else {
				AppendUtf8Codepoint(*target, charPressed, 36);
			}
		}
		return false;
	}

	if (focusedField < 0) return false;
	String* target = focusedField == 0 ? &dateInput : focusedField == 1 ? &startInput : &endInput;
	if (key == KEY_BACKSPACE) {
		if (!target->empty()) PopUtf8Char(*target);
		return false;
	}
	if (key == KEY_TAB) {
		focusedField = (focusedField + 1) % 3;
		return false;
	}
	if (charPressed >= 32 && charPressed <= 126 && target->size() < 16) {
		target->push_back(static_cast<char>(charPressed));
	}
	return false;
}

bool ManagementUI::handleClick(Vector2 mouse, int activeTab, Employee& currentUser) {
	if (activeTab == 3) {
		if (isMenuDialogOpen) {
			Rectangle dialog = {380, 135, 520, 460};
			if (!CheckCollisionPointRec(mouse, dialog)) {
				isMenuDialogOpen = false;
				return false;
			}
			if (CheckCollisionPointRec(mouse, {dialog.x + 30, dialog.y + 108, 460, 38})) menuDialogField = 0;
			if (CheckCollisionPointRec(mouse, {dialog.x + 30, dialog.y + 182, 460, 38})) menuDialogField = 1;

			const char* categories[] = {"Món chính", "Đồ uống", "Tráng miệng"};
			for (int c = 0; c < 3; ++c) {
				if (CheckCollisionPointRec(mouse, {dialog.x + 30 + c * 156.0f, dialog.y + 256, 146, 34})) {
					menuCategoryInput = categories[c];
				}
			}
			if (CheckCollisionPointRec(mouse, {dialog.x + 30, dialog.y + 328, 460, 38})) menuDialogField = 3;

			if (CheckCollisionPointRec(mouse, {dialog.x + 270, dialog.y + 402, 220, 40})) {
				isMenuDialogOpen = false;
				return false;
			}
			if (CheckCollisionPointRec(mouse, {dialog.x + 30, dialog.y + 402, 220, 40})) {
				return submitMenuDialog();
			}
			return false;
		}

		if (isIngredientDialogOpen) {
			Rectangle dialog = {380, 135, 520, 460};
			if (!CheckCollisionPointRec(mouse, dialog)) {
				isIngredientDialogOpen = false;
				return false;
			}
			if (CheckCollisionPointRec(mouse, {dialog.x + 30, dialog.y + 108, 460, 38})) ingDialogField = 0;
			if (CheckCollisionPointRec(mouse, {dialog.x + 30, dialog.y + 182, 460, 38})) ingDialogField = 1;

			const char* units[] = {"kg", "lít", "hộp", "gói"};
			for (int u = 0; u < 4; ++u) {
				if (CheckCollisionPointRec(mouse, {dialog.x + 30 + u * 116.0f, dialog.y + 256, 108, 34})) {
					ingUnitInput = units[u];
				}
			}
			if (CheckCollisionPointRec(mouse, {dialog.x + 30, dialog.y + 328, 460, 38})) ingDialogField = 3;

			if (CheckCollisionPointRec(mouse, {dialog.x + 270, dialog.y + 402, 220, 40})) {
				isIngredientDialogOpen = false;
				return false;
			}
			if (CheckCollisionPointRec(mouse, {dialog.x + 30, dialog.y + 402, 220, 40})) {
				return submitIngredientDialog();
			}
			return false;
		}

		if (CheckCollisionPointRec(mouse, {245, 154, 220, 34})) {
			managementSubTab = 0;
			return false;
		}
		if (CheckCollisionPointRec(mouse, {475, 154, 240, 34})) {
			managementSubTab = 1;
			return false;
		}

		if (managementSubTab == 0) {
			bool canEditMenu = currentUser.canManageAllBranches();
			const int visibleRows = 8;
			int total = products.size();
			int maxScroll = maxVal(0, total - visibleRows);

			if (total > visibleRows) {
				if (CheckCollisionPointRec(mouse, {945, 192, 36, 32})) {
					menuScrollOffset = maxVal(0, menuScrollOffset - 1);
					return false;
				}
				if (CheckCollisionPointRec(mouse, {987, 192, 36, 32})) {
					menuScrollOffset = minVal(maxScroll, menuScrollOffset + 1);
					return false;
				}
			}

			if (canEditMenu && CheckCollisionPointRec(mouse, {1035, 192, 210, 32})) {
				isMenuDialogOpen = true;
				menuDialogMode = 0;
				editingProductIndex = -1;
				int nextNum = 101 + products.size();
				menuCodeInput = String("M") + toString(nextNum);
				menuNameInput = "";
				menuCategoryInput = "Món chính";
				menuPriceInput = "";
				menuDialogField = 1;
				return false;
			}

			if (canEditMenu) {
				int rowY = 270;
				for (int r = 0; r < visibleRows; ++r) {
					int i = menuScrollOffset + r;
					if (i >= total) break;
					if (CheckCollisionPointRec(mouse, {1025, static_cast<float>(rowY + 8), 86, 30})) {
						isMenuDialogOpen = true;
						menuDialogMode = 1;
						editingProductIndex = i;
						menuCodeInput = products[i].code;
						menuNameInput = products[i].name;
						menuCategoryInput = products[i].category;
						menuPriceInput = toString(products[i].price);
						menuDialogField = 1;
						return false;
					}
					if (CheckCollisionPointRec(mouse, {1121, static_cast<float>(rowY + 8), 86, 30})) {
						String deletedName = products[i].name;
						String deletedCode = products[i].code;
						products.eraseAt(i);
						menuLookup.removeItem(deletedCode);
						menuScrollOffset = clampVal(menuScrollOffset, 0, maxVal(0, products.size() - visibleRows));
						message = String("Đã xoá món ăn: ") + deletedName;
						return true;
					}
					rowY += 48;
				}
			}
			return false;
		}

		bool isChainMgr = currentUser.canManageAllBranches();
		bool canAdjustStock = currentUser.canManageBranches();

		if (isChainMgr && CheckCollisionPointRec(mouse, {1015, 154, 230, 34})) {
			isIngredientDialogOpen = true;
			ingIdInput = "";
			ingNameInput = "";
			ingUnitInput = "kg";
			ingInitialStockInput = "50";
			ingDialogField = 0;
			return false;
		}

		int branchCardY = 198;
		if (isChainMgr && !branches.empty()) {
			const int maxVisBranches = 4;
			int totalBr = branches.size();
			float startX = 325.0f;
			if (totalBr > maxVisBranches) {
				if (CheckCollisionPointRec(mouse, {325, 198, 28, 28})) {
					invBranchScrollOffset = maxVal(0, invBranchScrollOffset - 1);
					return false;
				}
				startX = 360.0f;
			}
			int shown = minVal(totalBr - invBranchScrollOffset, maxVisBranches);
			for (int b = 0; b < shown; ++b) {
				int bIdx = invBranchScrollOffset + b;
				Rectangle bPill = {startX + b * 204.0f, 198.0f, 196.0f, 28.0f};
				if (CheckCollisionPointRec(mouse, bPill)) {
					selectedInventoryBranch = bIdx;
					return false;
				}
			}
			if (totalBr > maxVisBranches && CheckCollisionPointRec(mouse, {startX + shown * 204.0f, 198, 28, 28})) {
				invBranchScrollOffset = minVal(totalBr - maxVisBranches, invBranchScrollOffset + 1);
				return false;
			}
			branchCardY = 234;
		}

		if (branches.empty()) return false;
		Branch& activeBranch = branches[selectedInventoryBranch];

		int tableHeaderY = branchCardY + 62;
		int visibleIngRows = isChainMgr ? 6 : 7;
		int totalIng = ingredientTypes.size();
		int rowY = tableHeaderY + 38;

		for (int r = 0; r < visibleIngRows; ++r) {
			int idx = ingredientScrollOffset + r;
			if (idx >= totalIng) break;
			const IngredientType& ing = ingredientTypes[idx];

			if (canAdjustStock && CheckCollisionPointRec(mouse, {915, static_cast<float>(rowY + 8), 90, 30})) {
				activeBranch.addStock(ing.id, 10);
				message = String("Đã nhập +10 ") + ing.unit + " " + ing.name + " vào " + activeBranch.getName();
				return true;
			}
			if (canAdjustStock && activeBranch.getStock(ing.id) >= 5 &&
				CheckCollisionPointRec(mouse, {1015, static_cast<float>(rowY + 8), 90, 30})) {
				activeBranch.removeStock(ing.id, 5);
				message = String("Đã xuất -5 ") + ing.unit + " " + ing.name + " khỏi " + activeBranch.getName();
				return true;
			}
			if (isChainMgr && totalIng > 1 &&
				CheckCollisionPointRec(mouse, {1125, static_cast<float>(rowY + 8), 88, 30})) {
				String removedName = ing.name;
				ingredientTypes.eraseAt(idx);
				ingredientScrollOffset = clampVal(ingredientScrollOffset, 0, maxVal(0, ingredientTypes.size() - visibleIngRows));
				message = String("Đã xoá loại nguyên liệu: ") + removedName;
				return true;
			}
			rowY += 48;
		}
		return false;
	}

	if (activeTab == 4) {
		bool isChainMgr = currentUser.canManageAllBranches();
		bool canManage = currentUser.canManageBranches();

		if (CheckCollisionPointRec(mouse, {395, 166, 148, 36}) && canManage) { focusedField = 0; return false; }
		if (CheckCollisionPointRec(mouse, {551, 166, 118, 36}) && canManage) { focusedField = 1; return false; }
		if (CheckCollisionPointRec(mouse, {677, 166, 118, 36}) && canManage) { focusedField = 2; return false; }

		if (isChainMgr && !branches.empty() && CheckCollisionPointRec(mouse, {803, 166, 105, 36})) {
			scheduleBranchIndex = (scheduleBranchIndex + 1) % branches.size();
			return false;
		}

		if (CheckCollisionPointRec(mouse, {245, 166, 140, 36}) && canManage) {
			if (!validDate(dateInput) || !validTime(startInput) || !validTime(endInput) || startInput >= endInput) {
				message = "Ngày hoặc giờ không hợp lệ. Định dạng: YYYY-MM-DD và HH:MM.";
				return true;
			}
			String targetBranchId = currentUser.getBranchId();
			if (isChainMgr && !branches.empty()) {
				targetBranchId = branches[scheduleBranchIndex].getId();
			}
			scheduleService.openOpenShift(String("OPEN") + toString(scheduleService.getSchedules().size() + 1),
				dateInput, startInput, endInput, 30, targetBranchId, 2);
			message = String("Đã tạo ca làm việc mới cho chi nhánh ") + targetBranchId + "!";
			return true;
		}

		Rectangle checkBtn = isChainMgr ? Rectangle{916, 166, 329, 36} : Rectangle{805, 166, 260, 36};
		if (CheckCollisionPointRec(mouse, checkBtn) && canManage) {
			if (isChainMgr) {
				scheduleService.notifyShiftManager("ALL");
				message = "Đã kiểm tra độ phủ ca cho TOÀN BỘ các chi nhánh!";
			} else {
				scheduleService.notifyShiftManager(currentUser.getBranchId());
				message = String("Đã kiểm tra độ phủ ca cho chi nhánh ") + currentUser.getBranchId() + "!";
			}
			return true;
		}

		Vector<Schedule*> visibleSchedules;
		const Vector<Schedule*>& allSchedules = scheduleService.getSchedules();
		for (int i = 0; i < allSchedules.size(); ++i) {
			Schedule* schedule = allSchedules[i];
			if (isChainMgr || schedule->getBranchId() == "ALL" || schedule->getBranchId() == currentUser.getBranchId()) {
				visibleSchedules.push_back(schedule);
			}
		}

		const int maxVisibleShifts = 5;
		int totalShifts = visibleSchedules.size();
		String scheduleToDelete;
		for (int r = 0; r < maxVisibleShifts; ++r) {
			int idx = scheduleScrollOffset + r;
			if (idx >= totalShifts) break;
			Schedule* schedule = visibleSchedules[idx];
			int cardY = 262 + r * 82;

			if (schedule->isOpenShift() && CheckCollisionPointRec(mouse, {655, static_cast<float>(cardY + 20), 96, 32})) {
				message = currentUser.registerForEmptyShift(scheduleService, schedule->getId())
					? (String("Đăng ký ca ") + schedule->getId() + " thành công!")
					: String("Không thể đăng ký ca này (đã đăng ký hoặc khác chi nhánh)!");
				return true;
			}
			if (canManage && CheckCollisionPointRec(mouse, {761, static_cast<float>(cardY + 20), 96, 32})) {
				scheduleToDelete = schedule->getId();
				break;
			}
		}
		if (!scheduleToDelete.empty()) {
			message = scheduleService.deleteSchedule(scheduleToDelete)
				? (String("Đã xoá ca làm việc: ") + scheduleToDelete)
				: String("Không thể xoá ca làm việc!");
			return true;
		}
	}

	if (activeTab == 5) {
		if (CheckCollisionPointRec(mouse, {245, 172, 185, 40})) {
			Timekeeping* records = getTimekeeping(currentUser);
			if (records) message = records->checkIn("OPEN1", DataManager::currentTimestamp())
				? "Điểm danh vào ca (Check-in) thành công!" : "Bạn đã check-in trước đó!";
			return true;
		}
		if (CheckCollisionPointRec(mouse, {445, 172, 185, 40})) {
			Timekeeping* records = getTimekeeping(currentUser);
			if (records) message = records->checkOut("OPEN1", DataManager::currentTimestamp())
				? "Điểm danh ra ca (Check-out) thành công!" : "Không tìm thấy lượt vào ca hợp lệ!";
			return true;
		}
	}
	return false;
}

void ManagementUI::draw(Vector2 mouse, int activeTab, Employee& currentUser) {
	bool dialogOpen = isDialogOpen();
	if (dialogOpen && !wasDialogOpen) dialogOpenTime = GetTime();
	wasDialogOpen = dialogOpen;
	if (managementSubTab != lastSubTab) {
		subTabChangeTime = GetTime();
		lastSubTab = managementSubTab;
	}

	Vector2 bgMouse = dialogOpen ? Vector2{-10000.0f, -10000.0f} : mouse;

	if (activeTab == 3) {
		drawInventoryTab(bgMouse, currentUser);
		float t = static_cast<float>((GetTime() - subTabChangeTime) / 0.25);
		if (t < 1.0f) {
			float a = 1.0f - Anim::EaseOutCubic(t);
			DrawRectangle(221, 194, 1059, 528, Color{245, 241, 232, static_cast<unsigned char>(255.0f * a)});
		}
		drawMenuDialog(mouse);
		drawIngredientDialog(mouse);
	} else if (activeTab == 4) {
		drawScheduleTab(mouse, currentUser);
	} else if (activeTab == 5) {
		drawAttendanceTab(mouse, currentUser);
	}
}

const String& ManagementUI::getMessage() const {
	return message;
}

void ManagementUI::setMessage(const String& msg) {
	message = msg;
}
