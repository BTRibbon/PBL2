#include "ManagementUI.h"
#include "UIFont.h"

#include <cstdio>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <cctype>

namespace {
const Color darkGreen{35, 55, 53, 255};
const Color green{35, 105, 78, 255};
const Color muted{104, 115, 108, 255};
const Color panel{255, 255, 255, 255};
const Color panelAlt{248, 250, 248, 255};
const Color amber{180, 110, 40, 255};
const Color danger{195, 65, 50, 255};

static std::string formatMoney(int value) {
	std::string digits = std::to_string(value);
	for (int pos = static_cast<int>(digits.size()) - 3; pos > 0; pos -= 3)
		digits.insert(static_cast<std::size_t>(pos), ".");
	return digits + " đ";
}

static std::string currentTimestamp() {
	auto now = std::chrono::system_clock::now();
	std::time_t current = std::chrono::system_clock::to_time_t(now);
	std::tm localTime{};
#ifdef _WIN32
	localtime_s(&localTime, &current);
#else
	localtime_r(&current, &localTime);
#endif
	char buffer[32];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
	return buffer;
}

static bool validDate(const std::string& value) {
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

static bool validTime(const std::string& value) {
	if (value.size() != 5 || value[2] != ':') return false;
	if (!std::isdigit(static_cast<unsigned char>(value[0])) ||
		!std::isdigit(static_cast<unsigned char>(value[1])) ||
		!std::isdigit(static_cast<unsigned char>(value[3])) ||
		!std::isdigit(static_cast<unsigned char>(value[4]))) return false;
	int hours = std::stoi(value.substr(0, 2));
	int minutes = std::stoi(value.substr(3, 2));
	return hours < 24 && minutes < 60;
}
}

ManagementUI::ManagementUI(RestaurantChain& restaurantChain, std::vector<Branch>& branchValues,
	ScheduleService& service, ShiftManager& manager, std::vector<Timekeeping>& records,
	std::vector<Employee*>& employeeUsers, std::vector<Product>& productList, MenuLookup& lookup)
	: chain(restaurantChain), branches(branchValues), scheduleService(service),
	  shiftManager(manager), timekeepingRecords(records), employees(employeeUsers),
	  products(productList), menuLookup(lookup) {}

Timekeeping* ManagementUI::getTimekeeping(User& currentUser) const {
	Employee* currentEmployee = dynamic_cast<Employee*>(&currentUser);
	if (!currentEmployee) return nullptr;
	for (size_t i = 0; i < employees.size() && i < timekeepingRecords.size(); ++i)
		if (employees[i] == currentEmployee) return &timekeepingRecords[i];
	return nullptr;
}

void ManagementUI::drawButton(Rectangle bounds, const char* label, bool enabled, Color bgColor) const {
	DrawRectangleRounded(bounds, 0.25f, 4, enabled ? bgColor : Color{185, 195, 186, 255});
	DrawUiTextInRect(label, bounds, 13, RAYWHITE, 0, 6.0f);
}

void ManagementUI::drawManagementHeader(const char* title, const char* subtitle) const {
	std::string fullTitle = std::string(title) + " - " + chain.getName();
	DrawUiTextFitted(fullTitle.c_str(), 245, 104, 1000, 24, darkGreen);
	DrawUiTextFitted(subtitle, 245, 136, 1000, 13, muted);
}

void ManagementUI::drawMenuTab(Vector2 mouse, User& currentUser) {
	bool canEdit = currentUser.canManageBranches();

	std::string countText = "DANH SÁCH MÓN ĂN TRÊN TOÀN HỆ THỐNG (" + std::to_string(products.size()) + " món)";
	DrawUiTextFitted(countText.c_str(), 245, 200, 750, 14, darkGreen);

	if (canEdit) {
		drawButton({1065, 192, 180, 34}, "+ THÊM MÓN MỚI", true, green);
	}

	// Table header
	int tableY = 234;
	DrawRectangle(245, tableY, 1000, 34, Color{232, 238, 233, 255});
	DrawUiText("MÃ MÓN", 265, tableY + 9, 12, darkGreen);
	DrawUiText("TÊN MÓN ĂN", 375, tableY + 9, 12, darkGreen);
	DrawUiText("PHÂN LOẠI", 685, tableY + 9, 12, darkGreen);
	DrawUiText("ĐƠN GIÁ", 855, tableY + 9, 12, darkGreen);
	DrawUiText("THAO TÁC", 1045, tableY + 9, 12, darkGreen);

	int rowY = tableY + 38;
	int displayCount = std::min(static_cast<int>(products.size()), 8);
	for (int i = 0; i < displayCount; ++i) {
		DrawRectangle(245, rowY, 1000, 46, (i % 2 == 0) ? panel : panelAlt);
		DrawRectangleLines(245, rowY, 1000, 46, Color{235, 238, 235, 255});

		// Code badge
		Rectangle codeBadge = {262, static_cast<float>(rowY + 10), 82, 26};
		DrawRectangleRounded(codeBadge, 0.2f, 4, Color{240, 244, 242, 255});
		DrawUiTextInRect(products[i].code.c_str(), codeBadge, 13, darkGreen, 0, 4.0f);

		// Name (bounded to 290px)
		DrawUiTextFitted(products[i].name.c_str(), 375, rowY + 15, 290, 14, Color{45, 56, 50, 255});

		// Category pill
		Rectangle catBadge = {680, static_cast<float>(rowY + 10), 125, 26};
		DrawRectangleRounded(catBadge, 0.25f, 4, Color{238, 243, 240, 255});
		DrawUiTextInRect(products[i].category.c_str(), catBadge, 12, Color{60, 95, 75, 255}, 0, 6.0f);

		// Price (bounded to 150px)
		DrawUiTextFitted(formatMoney(products[i].price).c_str(), 855, rowY + 15, 150, 14, green);

		// Buttons
		if (canEdit) {
			drawButton({1030, static_cast<float>(rowY + 8), 88, 30}, "Sửa", true, amber);
			drawButton({1128, static_cast<float>(rowY + 8), 88, 30}, "Xoá", true, danger);
		} else {
			DrawUiText("Chỉ xem", 1055, rowY + 15, 13, muted);
		}

		rowY += 48;
	}

	if (!canEdit) {
		DrawUiTextFitted("Lưu ý: Chỉ Quản lý chi nhánh và Giám đốc chuỗi mới có quyền thêm, sửa, xoá món ăn.",
			245, rowY + 8, 980, 13, amber);
	}
	(void)mouse;
}

void ManagementUI::drawMenuDialog() {
	if (!isMenuDialogOpen) return;

	// Dark overlay
	DrawRectangle(0, 0, 1280, 760, Color{0, 0, 0, 135});

	// Dialog frame
	Rectangle dialog = {380, 140, 520, 450};
	DrawRectangleRounded(dialog, 0.05f, 8, panel);
	DrawRectangleRoundedLines(dialog, 0.05f, 8, green);

	const char* dialogTitle = menuDialogMode == 0 ? "THÊM MÓN ĂN MỚI" : "CHỈNH SỬA MÓN ĂN";
	DrawUiTextFitted(dialogTitle, dialog.x + 30, dialog.y + 24, 460, 20, darkGreen);
	DrawUiTextFitted("Điền đầy đủ thông tin bên dưới và nhấn Lưu", dialog.x + 30, dialog.y + 52, 460, 13, muted);

	// Field 0: Code
	DrawUiText("Mã món (Ví dụ: M109):", dialog.x + 30, dialog.y + 88, 13, darkGreen);
	Rectangle codeBox = {dialog.x + 30, dialog.y + 112, 460, 38};
	DrawRectangleRec(codeBox, Color{248, 250, 248, 255});
	DrawRectangleLinesEx(codeBox, 1.5f, menuDialogField == 0 ? green : Color{210, 218, 212, 255});
	DrawUiTextInRect(menuCodeInput.empty() ? "Nhập mã món..." : menuCodeInput.c_str(),
		codeBox, 14, menuCodeInput.empty() ? muted : darkGreen, -1, 12.0f);

	// Field 1: Name
	DrawUiText("Tên món ăn:", dialog.x + 30, dialog.y + 162, 13, darkGreen);
	Rectangle nameBox = {dialog.x + 30, dialog.y + 186, 460, 38};
	DrawRectangleRec(nameBox, Color{248, 250, 248, 255});
	DrawRectangleLinesEx(nameBox, 1.5f, menuDialogField == 1 ? green : Color{210, 218, 212, 255});
	DrawUiTextInRect(menuNameInput.empty() ? "Nhập tên món ăn..." : menuNameInput.c_str(),
		nameBox, 14, menuNameInput.empty() ? muted : darkGreen, -1, 12.0f);

	// Field 2: Category (Quick pills)
	DrawUiText("Phân loại:", dialog.x + 30, dialog.y + 236, 13, darkGreen);
	const char* categories[] = {"Món chính", "Đồ uống", "Tráng miệng"};
	for (int c = 0; c < 3; ++c) {
		Rectangle catPill = {dialog.x + 30 + c * 156.0f, dialog.y + 260, 146, 34};
		bool selected = menuCategoryInput == categories[c];
		DrawRectangleRounded(catPill, 0.25f, 4, selected ? green : Color{235, 242, 237, 255});
		DrawUiTextInRect(categories[c], catPill, 13, selected ? RAYWHITE : darkGreen, 0, 8.0f);
	}

	// Field 3: Price
	DrawUiText("Đơn giá (VNĐ):", dialog.x + 30, dialog.y + 310, 13, darkGreen);
	Rectangle priceBox = {dialog.x + 30, dialog.y + 334, 460, 38};
	DrawRectangleRec(priceBox, Color{248, 250, 248, 255});
	DrawRectangleLinesEx(priceBox, 1.5f, menuDialogField == 3 ? green : Color{210, 218, 212, 255});
	DrawUiTextInRect(menuPriceInput.empty() ? "Nhập giá bán (VD: 35000)..." : menuPriceInput.c_str(),
		priceBox, 14, menuPriceInput.empty() ? muted : darkGreen, -1, 12.0f);

	// Actions
	drawButton({dialog.x + 30, dialog.y + 390, 220, 40}, "LƯU MÓN ĂN", true, green);
	drawButton({dialog.x + 270, dialog.y + 390, 220, 40}, "HỦY BỎ", true, Color{150, 160, 152, 255});
}

void ManagementUI::drawInventoryTab(Vector2 mouse, User& currentUser) {
	drawManagementHeader("Quản lý Kho & Thực đơn", "Quản lý danh sách món ăn, giá bán và tồn kho nguyên liệu chi nhánh");

	// Sub-tab toggles
	Rectangle tabMenuRec = {245, 154, 220, 34};
	Rectangle tabStockRec = {475, 154, 240, 34};
	DrawRectangleRounded(tabMenuRec, 0.25f, 4, managementSubTab == 0 ? green : Color{225, 233, 226, 255});
	DrawUiTextInRect("1. THỰC ĐƠN MÓN ĂN", tabMenuRec, 13, managementSubTab == 0 ? RAYWHITE : darkGreen, 0, 8.0f);

	DrawRectangleRounded(tabStockRec, 0.25f, 4, managementSubTab == 1 ? green : Color{225, 233, 226, 255});
	DrawUiTextInRect("2. KHO NGUYÊN LIỆU", tabStockRec, 13, managementSubTab == 1 ? RAYWHITE : darkGreen, 0, 8.0f);

	if (managementSubTab == 0) {
		drawMenuTab(mouse, currentUser);
		drawMenuDialog();
		return;
	}

	// Sub-tab 1: Inventory per branch
	int visibleRow = 0;
	int startY = 202;
	for (int i = 0; i < static_cast<int>(branches.size()); ++i) {
		Employee* currentEmployee = dynamic_cast<Employee*>(&currentUser);
		if (currentEmployee && !currentUser.canManageAllBranches() &&
			branches[i].getId() != currentEmployee->branchId) continue;

		int y = startY + visibleRow * 105;
		Rectangle card = {245, static_cast<float>(y), 1000, 92};
		DrawRectangleRounded(card, 0.08f, 4, panel);
		DrawRectangleRoundedLines(card, 0.08f, 4, Color{225, 233, 226, 255});

		// Branch name (bounded to 470px before buttons)
		DrawUiTextFitted((branches[i].getName() + " (" + branches[i].getId() + ")").c_str(),
			265, y + 14, 470, 16, darkGreen);

		// Details (bounded to 470px)
		char details1[160];
		std::snprintf(details1, sizeof(details1), "Tồn kho: Bột mì %d kg  •  Mì sợi %d kg",
			branches[i].getStock("FLOUR"), branches[i].getStock("NOODLES"));
		DrawUiTextFitted(details1, 265, y + 40, 470, 13, green);

		char details2[160];
		std::snprintf(details2, sizeof(details2), "Giờ hoạt động: %s - %s  •  Doanh thu: %s",
			branches[i].getOpeningHours().getOpenTime().c_str(),
			branches[i].getOpeningHours().getCloseTime().c_str(),
			formatMoney(branches[i].getRevenue()).c_str());
		DrawUiTextFitted(details2, 265, y + 62, 470, 12, muted);

		// Controls (x = 755..1230)
		drawButton({755.0f, static_cast<float>(y + 14), 110, 30}, "+10kg Bột", true, green);
		drawButton({873.0f, static_cast<float>(y + 14), 110, 30}, "-5kg Bột", true, amber);
		drawButton({991.0f, static_cast<float>(y + 14), 110, 30}, "+10kg Mì", true, green);
		drawButton({1109.0f, static_cast<float>(y + 14), 116, 30}, "-5kg Mì", true, amber);

		drawButton({991.0f, static_cast<float>(y + 52), 234, 28}, "Đặt giờ chuẩn (08:00 - 22:30)", true, darkGreen);
		visibleRow++;
	}
	(void)mouse;
}

void ManagementUI::drawScheduleTab(Vector2 mouse, User& currentUser) {
	drawManagementHeader("Lịch làm việc & Phân ca", "Tạo ca làm việc mở, đăng ký ca trực và kiểm tra độ phủ nhân sự");

	bool isEmployee = currentUser.getRole() == Role::Employee ||
		currentUser.getRole() == Role::Manager || currentUser.getRole() == Role::BranchManager;

	std::string dateLabel = "Ngày: " + dateInput;
	std::string startLabel = "Vào: " + startInput;
	std::string endLabel = "Ra: " + endInput;

	drawButton({245, 168, 145, 36}, "+ TẠO CA LÀM", currentUser.canManageBranches(), green);

	Rectangle dBox = {402, 168, 155, 36};
	Rectangle sBox = {569, 168, 135, 36};
	Rectangle eBox = {716, 168, 135, 36};
	DrawRectangleRec(dBox, panel);
	DrawRectangleRec(sBox, panel);
	DrawRectangleRec(eBox, panel);
	DrawRectangleLinesEx(dBox, 1.5f, focusedField == 0 ? green : Color{202, 211, 201, 255});
	DrawRectangleLinesEx(sBox, 1.5f, focusedField == 1 ? green : Color{202, 211, 201, 255});
	DrawRectangleLinesEx(eBox, 1.5f, focusedField == 2 ? green : Color{202, 211, 201, 255});

	DrawUiTextInRect(dateLabel.c_str(), dBox, 13, darkGreen, -1, 10.0f);
	DrawUiTextInRect(startLabel.c_str(), sBox, 13, darkGreen, -1, 10.0f);
	DrawUiTextInRect(endLabel.c_str(), eBox, 13, darkGreen, -1, 10.0f);

	drawButton({865, 168, 215, 36}, "KIỂM TRA ĐỘ PHỦ CA", currentUser.canManageBranches(), darkGreen);

	DrawUiTextFitted("Nhấp vào ô để nhập: Ngày (YYYY-MM-DD), Giờ bắt đầu (HH:MM), Giờ kết thúc (HH:MM).",
		245, 212, 980, 12, muted);

	DrawUiText("DANH SÁCH CA LÀM VIỆC", 245, 238, 14, darkGreen);

	// Shift cards on the left (x = 245..885, width = 640)
	int y = 264;
	int shown = 0;
	for (const auto& schedule : scheduleService.getSchedules()) {
		if (shown >= 5) break;
		DrawRectangleRounded({245, static_cast<float>(y), 640, 72}, 0.08f, 4, panel);
		DrawRectangleRoundedLines({245, static_cast<float>(y), 640, 72}, 0.08f, 4, Color{225, 233, 226, 255});

		char details[200];
		std::snprintf(details, sizeof(details), "Mã ca: %s  |  Ngày: %s  |  %s - %s  |  CN: %s",
			schedule->getId().c_str(), schedule->getDate().c_str(),
			schedule->getStartTime().c_str(), schedule->getEndTime().c_str(),
			schedule->getBranchId().c_str());
		DrawUiTextFitted(details, 262, y + 14, 390, 13, darkGreen);

		bool covered = schedule->hasCoverage();
		std::snprintf(details, sizeof(details), "Nhân sự: %d/%d  •  %s",
			schedule->getAssignedEmployeeCount(), schedule->getRequiredEmployees(),
			covered ? "Đã đủ người" : "Đang thiếu người");
		DrawUiTextFitted(details, 262, y + 42, 390, 13, covered ? green : amber);

		if (isEmployee && schedule->isOpenShift())
			drawButton({665, static_cast<float>(y + 20), 96, 32}, "Đăng ký", true, green);
		if (currentUser.canManageBranches())
			drawButton({771, static_cast<float>(y + 20), 96, 32}, "Xoá ca", true, danger);

		y += 82;
		shown++;
	}

	// Right Panel: Status & Coverage Alerts (x = 900..1245, width = 345)
	Rectangle rightPanel = {900, 264, 345, 410};
	DrawRectangleRounded(rightPanel, 0.06f, 4, panel);
	DrawRectangleRoundedLines(rightPanel, 0.06f, 4, Color{225, 233, 226, 255});

	DrawUiText("CHỈ TIÊU & CẢNH BÁO", rightPanel.x + 18, rightPanel.y + 16, 14, darkGreen);
	DrawLine(static_cast<int>(rightPanel.x + 18), static_cast<int>(rightPanel.y + 42),
		static_cast<int>(rightPanel.x + rightPanel.width - 18), static_cast<int>(rightPanel.y + 42),
		Color{230, 236, 231, 255});

	Employee* currentEmployee = dynamic_cast<Employee*>(&currentUser);
	bool minSatisfied = currentEmployee && scheduleService.validateMinimumShifts(*currentEmployee, "2026-01-01", "2026-12-31");
	Rectangle statusBadge = {rightPanel.x + 18, rightPanel.y + 54, rightPanel.width - 36, 36};
	DrawRectangleRounded(statusBadge, 0.2f, 4, minSatisfied ? Color{232, 245, 238, 255} : Color{252, 242, 230, 255});
	DrawUiTextInRect(minSatisfied ? "Chỉ tiêu ca tối thiểu: ĐẠT" : "Chỉ tiêu ca tối thiểu: CHƯA ĐẠT",
		statusBadge, 13, minSatisfied ? green : amber, 0, 8.0f);

	DrawUiText("Danh sách cảnh báo thiếu ca:", rightPanel.x + 18, rightPanel.y + 106, 13, muted);
	float alertY = rightPanel.y + 132;
	if (shiftManager.getAlerts().empty()) {
		DrawUiTextFitted("Không có cảnh báo. Mọi ca đã ổn định.",
			rightPanel.x + 18, alertY, rightPanel.width - 36, 13, green);
	} else {
		for (const std::string& alert : shiftManager.getAlerts()) {
			if (alertY + 36 > rightPanel.y + rightPanel.height - 12) break;
			DrawUiTextWrapped(alert.c_str(), {rightPanel.x + 18, alertY, rightPanel.width - 36, 38}, 12, amber);
			alertY += 42;
		}
	}

	(void)mouse;
}

void ManagementUI::drawAttendanceTab(Vector2 mouse, User& currentUser) {
	drawManagementHeader("Điểm danh & Chấm công", "Ghi nhận giờ vào ca và ra ca theo thời gian hệ thống thực tế");

	bool isEmployee = currentUser.getRole() == Role::Employee ||
		currentUser.getRole() == Role::Manager || currentUser.getRole() == Role::BranchManager;

	drawButton({245, 172, 185, 40}, "VÀO CA (CHECK-IN)", isEmployee, green);
	drawButton({445, 172, 185, 40}, "RA CA (CHECK-OUT)", isEmployee, amber);

	Timekeeping* records = getTimekeeping(currentUser);
	std::string empInfo = "Nhân sự: " + currentUser.getName() + "  •  Mã NV: " +
		(records ? records->getEmployeeId() : currentUser.getBranchId());
	DrawUiTextFitted(empInfo.c_str(), 245, 232, 980, 15, darkGreen);

	DrawUiText("LỊCH SỬ ĐIỂM DANH GẦN ĐÂY", 245, 266, 13, muted);

	int tableY = 292;
	DrawRectangle(245, tableY, 1000, 34, Color{232, 238, 233, 255});
	DrawUiText("MÃ CA TRỰC", 265, tableY + 9, 12, darkGreen);
	DrawUiText("THỜI GIAN VÀO CA", 430, tableY + 9, 12, darkGreen);
	DrawUiText("THỜI GIAN RA CA", 690, tableY + 9, 12, darkGreen);
	DrawUiText("TRẠNG THÁI", 960, tableY + 9, 12, darkGreen);

	int y = tableY + 38;
	if (records) {
		int count = 0;
		for (const AttendanceRecord& record : records->getRecords()) {
			if (count >= 8) break;
			DrawRectangle(245, y, 1000, 40, (count % 2 == 0) ? panel : panelAlt);
			DrawRectangleLines(245, y, 1000, 40, Color{225, 233, 226, 255});

			DrawUiTextFitted(record.scheduleId.c_str(), 265, y + 11, 145, 13, darkGreen);
			DrawUiTextFitted(record.checkIn.empty() ? "---" : record.checkIn.c_str(), 430, y + 11, 240, 13, darkGreen);
			DrawUiTextFitted(record.checkOut.empty() ? "Đang trong ca..." : record.checkOut.c_str(), 690, y + 11, 240, 13,
				record.checkOut.empty() ? amber : darkGreen);
			DrawUiTextFitted(record.isPresent ? "Có mặt đầy đủ" : "Chưa hoàn tất", 960, y + 11, 260, 13,
				record.isPresent ? green : amber);
			y += 44;
			count++;
		}
	}
	(void)mouse;
}

void ManagementUI::updateInput(int key, int charPressed) {
	if (isMenuDialogOpen) {
		std::string* target = nullptr;
		if (menuDialogField == 0) target = &menuCodeInput;
		else if (menuDialogField == 1) target = &menuNameInput;
		else if (menuDialogField == 2) target = &menuCategoryInput;
		else if (menuDialogField == 3) target = &menuPriceInput;

		if (key == KEY_BACKSPACE && target && !target->empty()) {
			PopUtf8Char(*target);
			return;
		}
		if (key == KEY_TAB) {
			menuDialogField = (menuDialogField + 1) % 4;
			return;
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
		return;
	}

	if (focusedField < 0) return;
	std::string* target = focusedField == 0 ? &dateInput : focusedField == 1 ? &startInput : &endInput;
	if (key == KEY_BACKSPACE) {
		if (!target->empty()) PopUtf8Char(*target);
		return;
	}
	if (charPressed >= 32 && charPressed <= 126 && target->size() < 16) {
		target->push_back(static_cast<char>(charPressed));
	}
}

bool ManagementUI::handleClick(Vector2 mouse, int activeTab, User& currentUser) {
	if (activeTab == 3) {
		// Handle Dialog if open
		if (isMenuDialogOpen) {
			Rectangle dialog = {380, 140, 520, 450};
			if (CheckCollisionPointRec(mouse, {dialog.x + 30, dialog.y + 112, 460, 38})) menuDialogField = 0;
			if (CheckCollisionPointRec(mouse, {dialog.x + 30, dialog.y + 186, 460, 38})) menuDialogField = 1;

			const char* categories[] = {"Món chính", "Đồ uống", "Tráng miệng"};
			for (int c = 0; c < 3; ++c) {
				if (CheckCollisionPointRec(mouse, {dialog.x + 30 + c * 156.0f, dialog.y + 260, 146, 34})) {
					menuCategoryInput = categories[c];
				}
			}
			if (CheckCollisionPointRec(mouse, {dialog.x + 30, dialog.y + 334, 460, 38})) menuDialogField = 3;

			// Cancel button
			if (CheckCollisionPointRec(mouse, {dialog.x + 270, dialog.y + 390, 220, 40})) {
				isMenuDialogOpen = false;
				return false;
			}

			// Save button
			if (CheckCollisionPointRec(mouse, {dialog.x + 30, dialog.y + 390, 220, 40})) {
				if (menuCodeInput.empty() || menuNameInput.empty() || menuPriceInput.empty()) {
					message = "Vui lòng nhập đầy đủ thông tin món ăn!";
					return true;
				}
				int price = 0;
				try {
					price = std::stoi(menuPriceInput);
				} catch (...) {
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
					Product newProd{menuCodeInput, menuNameInput, menuCategoryInput, price, col};
					products.push_back(newProd);
					menuLookup.addItem({menuCodeInput, menuNameInput, static_cast<float>(price)});
					message = "Đã thêm món mới: " + menuNameInput;
				} else if (menuDialogMode == 1 && editingProductIndex >= 0 &&
						   editingProductIndex < static_cast<int>(products.size())) {
					products[editingProductIndex].code = menuCodeInput;
					products[editingProductIndex].name = menuNameInput;
					products[editingProductIndex].category = menuCategoryInput;
					products[editingProductIndex].price = price;
					products[editingProductIndex].color = col;
					menuLookup.addItem({menuCodeInput, menuNameInput, static_cast<float>(price)});
					message = "Đã cập nhật món: " + menuNameInput;
				}
				isMenuDialogOpen = false;
				return true;
			}
			return false;
		}

		// Sub-tab toggling
		if (CheckCollisionPointRec(mouse, {245, 154, 220, 34})) {
			managementSubTab = 0;
			return false;
		}
		if (CheckCollisionPointRec(mouse, {475, 154, 240, 34})) {
			managementSubTab = 1;
			return false;
		}

		// In Sub-tab 0: Menu management
		if (managementSubTab == 0) {
			bool canEdit = currentUser.canManageBranches();
			if (canEdit && CheckCollisionPointRec(mouse, {1065, 192, 180, 34})) {
				isMenuDialogOpen = true;
				menuDialogMode = 0;
				editingProductIndex = -1;
				menuCodeInput = "M" + std::to_string(101 + products.size());
				menuNameInput = "";
				menuCategoryInput = "Món chính";
				menuPriceInput = "";
				menuDialogField = 1;
				return false;
			}

			if (canEdit) {
				int rowY = 272;
				int displayCount = std::min(static_cast<int>(products.size()), 8);
				for (int i = 0; i < displayCount; ++i) {
					if (CheckCollisionPointRec(mouse, {1030, static_cast<float>(rowY + 8), 88, 30})) {
						isMenuDialogOpen = true;
						menuDialogMode = 1;
						editingProductIndex = i;
						menuCodeInput = products[i].code;
						menuNameInput = products[i].name;
						menuCategoryInput = products[i].category;
						menuPriceInput = std::to_string(products[i].price);
						menuDialogField = 1;
						return false;
					}
					if (CheckCollisionPointRec(mouse, {1128, static_cast<float>(rowY + 8), 88, 30})) {
						std::string deletedName = products[i].name;
						products.erase(products.begin() + i);
						message = "Đã xoá món ăn: " + deletedName;
						return true;
					}
					rowY += 48;
				}
			}
			return false;
		}

		// Sub-tab 1: Inventory
		int visibleRow = 0;
		int startY = 202;
		for (int i = 0; i < static_cast<int>(branches.size()); ++i) {
			Employee* currentEmployee = dynamic_cast<Employee*>(&currentUser);
			if (currentEmployee && !currentUser.canManageAllBranches() && branches[i].getId() != currentEmployee->branchId) continue;
			int y = startY + visibleRow * 105;

			if (CheckCollisionPointRec(mouse, {755.0f, static_cast<float>(y + 14), 110, 30})) {
				branches[i].addStock("FLOUR", 10);
				message = "Đã nhập thêm 10kg bột mì cho " + branches[i].getName();
				return true;
			}
			if (CheckCollisionPointRec(mouse, {873.0f, static_cast<float>(y + 14), 110, 30})) {
				branches[i].removeStock("FLOUR", 5);
				message = "Đã xuất 5kg bột mì cho " + branches[i].getName();
				return true;
			}
			if (CheckCollisionPointRec(mouse, {991.0f, static_cast<float>(y + 14), 110, 30})) {
				branches[i].addStock("NOODLES", 10);
				message = "Đã nhập thêm 10kg mì sợi cho " + branches[i].getName();
				return true;
			}
			if (CheckCollisionPointRec(mouse, {1109.0f, static_cast<float>(y + 14), 116, 30})) {
				branches[i].removeStock("NOODLES", 5);
				message = "Đã xuất 5kg mì sợi cho " + branches[i].getName();
				return true;
			}
			if (CheckCollisionPointRec(mouse, {991.0f, static_cast<float>(y + 52), 234, 28})) {
				branches[i].setOpeningHours("08:00", "22:30");
				message = "Đã cập nhật giờ mở cửa chuẩn (08:00 - 22:30) cho " + branches[i].getName();
				return true;
			}
			visibleRow++;
		}
	}

	if (activeTab == 4) {
		if (CheckCollisionPointRec(mouse, {245, 168, 145, 36}) && currentUser.canManageBranches()) {
			if (!validDate(dateInput) || !validTime(startInput) || !validTime(endInput) || startInput >= endInput) {
				message = "Ngày hoặc giờ không hợp lệ. Định dạng: YYYY-MM-DD và HH:MM.";
				return true;
			}
			Employee* currentEmployee = dynamic_cast<Employee*>(&currentUser);
			scheduleService.openOpenShift("OPEN" + std::to_string(scheduleService.getSchedules().size() + 1),
				dateInput, startInput, endInput, 30, currentEmployee ? currentEmployee->branchId : "BR001", 2);
			message = "Đã tạo ca làm việc mở mới thành công!";
			return true;
		}
		if (CheckCollisionPointRec(mouse, {402, 168, 155, 36}) && currentUser.canManageBranches()) focusedField = 0;
		if (CheckCollisionPointRec(mouse, {569, 168, 135, 36}) && currentUser.canManageBranches()) focusedField = 1;
		if (CheckCollisionPointRec(mouse, {716, 168, 135, 36}) && currentUser.canManageBranches()) focusedField = 2;
		if (CheckCollisionPointRec(mouse, {865, 168, 215, 36}) && currentUser.canManageBranches()) {
			scheduleService.notifyShiftManager();
			message = "Đã làm mới và kiểm tra cảnh báo phân ca!";
			return true;
		}

		int scheduleIndex = 0;
		std::string scheduleToDelete;
		for (const auto& schedule : scheduleService.getSchedules()) {
			if (scheduleIndex >= 5) break;
			int cardY = 264 + scheduleIndex * 82;
			if (schedule->isOpenShift() && CheckCollisionPointRec(mouse, {665, static_cast<float>(cardY + 20), 96, 32})) {
				Employee* currentEmployee = dynamic_cast<Employee*>(&currentUser);
				message = currentEmployee && currentEmployee->registerForEmptyShift(scheduleService, schedule->getId())
					? "Đăng ký ca làm việc thành công!" : "Không thể đăng ký ca làm việc này!";
				return true;
			}
			if (currentUser.canManageBranches() &&
				CheckCollisionPointRec(mouse, {771, static_cast<float>(cardY + 20), 96, 32})) {
				scheduleToDelete = schedule->getId();
				break;
			}
			++scheduleIndex;
		}
		if (!scheduleToDelete.empty()) {
			message = scheduleService.deleteSchedule(scheduleToDelete)
				? "Đã xoá ca làm việc: " + scheduleToDelete
				: "Không thể xoá ca làm việc!";
			return true;
		}
	}

	if (activeTab == 5) {
		if (CheckCollisionPointRec(mouse, {245, 172, 185, 40}) &&
			(currentUser.getRole() == Role::Employee || currentUser.getRole() == Role::Manager ||
			 currentUser.getRole() == Role::BranchManager)) {
			Timekeeping* records = getTimekeeping(currentUser);
			if (records) message = records->checkIn("OPEN1", currentTimestamp())
				? "Điểm danh vào ca (Check-in) thành công!" : "Bạn đã check-in trước đó!";
			return true;
		}
		if (CheckCollisionPointRec(mouse, {445, 172, 185, 40}) &&
			(currentUser.getRole() == Role::Employee || currentUser.getRole() == Role::Manager ||
			 currentUser.getRole() == Role::BranchManager)) {
			Timekeeping* records = getTimekeeping(currentUser);
			if (records) message = records->checkOut("OPEN1", currentTimestamp())
				? "Điểm danh ra ca (Check-out) thành công!" : "Không tìm thấy lượt vào ca hợp lệ!";
			return true;
		}
	}
	return false;
}

void ManagementUI::draw(Vector2 mouse, int activeTab, User& currentUser) {
	if (activeTab == 3) drawInventoryTab(mouse, currentUser);
	else if (activeTab == 4) drawScheduleTab(mouse, currentUser);
	else if (activeTab == 5) drawAttendanceTab(mouse, currentUser);
}

const std::string& ManagementUI::getMessage() const {
	return message;
}

void ManagementUI::setMessage(const std::string& msg) {
	message = msg;
}
