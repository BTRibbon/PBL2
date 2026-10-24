#include "raylib.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>

#include "datastructures/MenuLookup.h"
#include "datastructures/OrderQueue.h"
#include "datastructures/String.h"
#include "datastructures/Vector.h"
#include "models/AccountRecord.h"
#include "models/Branch.h"
#include "models/Employee.h"
#include "models/Order.h"
#include "models/RestaurantChain.h"
#include "models/Timekeeping.h"
#include "services/DataManager.h"
#include "services/ScheduleService.h"
#include "services/ShiftManager.h"
#include "UI/DutMapUI.h"
#include "UI/ManagementUI.h"
#include "UI/PosUI.h"
#include "UI/ReportUI.h"
#include "UI/StaffManagementUI.h"
#include "UI/UIAnim.h"
#include "UI/UIFont.h"

namespace {

struct NavItem {
	int tabId;
	const char* label;
	const char* hint;
};

// Xây dựng danh sách menu điều hướng theo quyền đa hình của nhân sự đang đăng nhập
Vector<NavItem> buildNavItems(const Employee* user) {
	static const NavItem kAllTabs[] = {
		{0, "Bán hàng (POS)", "Tạo đơn hàng và thanh toán tại quầy"},
		{1, "Báo cáo doanh thu", "Xem thống kê doanh thu và đơn hàng"},
		{2, "Bản đồ & Chi nhánh", "Tìm đường giao hàng và quản lý chi nhánh"},
		{3, "Thực đơn & Kho", "Quản lý món ăn và tồn kho nguyên liệu"},
		{4, "Lịch làm & Chấm công", "Phân ca làm việc và chấm công nhân viên"},
		{5, "Trạng thái chi nhánh", "Cấu hình giờ mở cửa chi nhánh"},
		{6, "Quản lý Nhân sự", "Thêm, sửa, xoá tài khoản nhân viên & quản lý"}
	};
	Vector<NavItem> result;
	if (!user) return result;
	for (int i = 0; i < 7; ++i) {
		if (user->canAccessTab(kAllTabs[i].tabId)) {
			result.push_back(kAllTabs[i]);
		}
	}
	return result;
}

// Thiết lập thư mục làm việc về đúng thư mục chứa file thực thi (app/)
bool ensureApplicationWorkingDirectory() {
	const char* appDir = GetApplicationDirectory();
	if (appDir && appDir[0] != '\0') {
		ChangeDirectory(appDir);
	}
	if (FileExists("data/accounts.dat")) return true;
	if (ChangeDirectory("../app") && FileExists("data/accounts.dat")) return true;
	return false;
}

} // namespace

int main() {
	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 760, "Bếp Nhỏ - Hệ Thống Quản Lý Bán Hàng & Chuỗi Nhà Hàng");
	SetWindowMinSize(960, 620);

	if (!ensureApplicationWorkingDirectory()) {
		std::fprintf(stderr, "Không tìm thấy dữ liệu mã hoá data/accounts.dat trong thư mục ứng dụng.\n");
		CloseWindow();
		return 1;
	}

	InitUiFont();
	SetTargetFPS(60);

	// 1. Nạp dữ liệu tài khoản & khởi tạo cây phân cấp kế thừa Employee -> Manager -> ChainManager
	Vector<AccountRecord> accounts;
	if (!DataManager::loadAccounts(accounts)) {
		std::fprintf(stderr, "Không thể giải mã data/accounts.dat.\n");
		UnloadUiFont();
		CloseWindow();
		return 1;
	}

	Employee* currentUser = nullptr;
	Vector<Employee*> employeePool;
	DataManager::rebuildEmployeePool(accounts, employeePool, currentUser);

	// 2. Nạp dữ liệu chấm công, chi nhánh, đơn hàng, thực đơn, nguyên liệu, lịch làm việc & kho
	Vector<Timekeeping> timekeepingRecords;
	for (int i = 0; i < employeePool.size(); ++i) {
		timekeepingRecords.push_back(Timekeeping(employeePool[i]->getId()));
	}
	DataManager::loadAttendance(timekeepingRecords);

	RestaurantChain chain("Bếp Nhỏ");
	Vector<Branch> branchValues;
	DataManager::loadBranches(branchValues, chain);

	OrderQueue orderQueue;
	Vector<Order> orders;
	DataManager::loadOrders(orders);
	for (int i = 0; i < orders.size(); ++i) {
		if (orders[i].status == "pending") orderQueue.push(orders[i]);
	}

	MenuLookup menuLookup;
	Vector<Product> products;
	DataManager::loadMenu(products, menuLookup);

	Vector<IngredientType> ingredientTypes;
	DataManager::loadIngredients(ingredientTypes);

	ShiftManager shiftManager;
	ScheduleService scheduleService(&shiftManager);
	DataManager::loadSchedules(scheduleService);
	DataManager::loadInventory(branchValues);

	// 3. Khởi tạo các phân hệ giao diện (UI Modules)
	PosUI posUi;
	posUi.syncNextOrderId(orders);
	ReportUI reportUi;
	DutMapUI dutMapUi;
	StaffManagementUI staffManagementUi;
	ManagementUI managementUi(chain, branchValues, scheduleService, shiftManager,
		timekeepingRecords, employeePool, products, menuLookup, ingredientTypes);

	// 4. Trạng thái phiên đăng nhập & điều hướng
	bool loginDialog = true;
	int loginField = 0;
	String usernameInput;
	String passwordInput;
	int activeTab = 0;

	const String dateLabel = DataManager::currentDateLabel();
	float animationTime = 0.0f;
	String statusMessage = "Vui lòng đăng nhập để sử dụng hệ thống";
	int statusVersion = 0;
	int lastStatusVersion = 0;
	double statusChangeTime = -10.0;
	Anim::ToastType statusType = Anim::ToastType::Info;

	int lastActiveTab = -1;
	double tabChangeTime = -10.0;
	double loginShakeTime = -10.0;
	double loginOpenTime = 0.0;
	bool wasLoginDialog = true;
	float navIndicatorY = -1.0f;

	const Rectangle loginBoxRec = {380, 195, 520, 330};
	const Rectangle loginUserBoxRec = {415, 308, 450, 42};
	const Rectangle loginPassBoxRec = {415, 384, 450, 42};
	const Rectangle loginBtnRec = {415, 450, 450, 46};

	auto attemptLogin = [&]() {
		for (int i = 0; i < accounts.size(); ++i) {
			AccountRecord& account = accounts[i];
			if (account.username == usernameInput &&
				HashPassword(passwordInput, account.salt) == account.passwordHash) {
				currentUser = account.user;
				activeTab = currentUser->canAccessTab(0) ? 0 : 1;
				loginDialog = false;
				usernameInput.clear();
				passwordInput.clear();
				statusVersion++;
				statusMessage = String("Đăng nhập thành công! Chức vụ: ") + currentUser->getRoleTitle();
				return;
			}
		}
		statusVersion++;
		statusMessage = "Sai tên đăng nhập hoặc mật khẩu!";
		passwordInput.clear();
	};

	while (!WindowShouldClose()) {
		animationTime += GetFrameTime();
		Anim::BeginFrame();

		float scale = minVal(GetScreenWidth() / 1280.0f, GetScreenHeight() / 760.0f);
		Vector2 mouse = GetMousePosition();
		mouse.x = (mouse.x - GetScreenWidth() / 2.0f) / scale + 640.0f;
		mouse.y = (mouse.y - GetScreenHeight() / 2.0f) / scale + 380.0f;

		String hoverHint;
		bool dialogConsumed = loginDialog || managementUi.isDialogOpen() ||
			dutMapUi.isModalOpen() || staffManagementUi.isModalOpen();

		// Xử lý cuộn chuột (Mouse Wheel)
		float wheel = GetMouseWheelMove();
		if (std::fabs(wheel) > 0.01f && !dialogConsumed && currentUser) {
			if (activeTab == 0) posUi.handleScroll(mouse, wheel, products);
			else if (activeTab == 1) reportUi.handleScroll(mouse, wheel, branchValues, products, *currentUser);
			else if (activeTab == 2) dutMapUi.handleScroll(mouse, wheel);
			else if (activeTab >= 3 && activeTab <= 5) managementUi.updateScroll(mouse, wheel, activeTab, *currentUser);
			else if (activeTab == 6) staffManagementUi.handleScroll(mouse, wheel);
		}

		Vector<NavItem> navItems = buildNavItems(currentUser);

		// Xử lý sự kiện nhấp chuột trái
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			if (loginDialog) {
				if (CheckCollisionPointRec(mouse, loginUserBoxRec)) loginField = 0;
				if (CheckCollisionPointRec(mouse, loginPassBoxRec)) loginField = 1;
				if (CheckCollisionPointRec(mouse, loginBtnRec)) attemptLogin();
			} else if (staffManagementUi.isModalOpen()) {
				if (staffManagementUi.handleClick(mouse, accounts, branchValues, *currentUser, statusMessage, statusVersion)) {
					DataManager::rebuildEmployeePool(accounts, employeePool, currentUser);
				}
			} else if (dutMapUi.isModalOpen()) {
				if (dutMapUi.handleClick(mouse, branchValues, chain, accounts, currentUser, statusMessage, statusVersion)) {
					DataManager::saveBranches(branchValues);
					DataManager::saveInventory(branchValues, ingredientTypes);
					DataManager::rebuildEmployeePool(accounts, employeePool, currentUser);
				}
			} else if (managementUi.isDialogOpen()) {
				if (managementUi.handleClick(mouse, activeTab, *currentUser)) {
					DataManager::saveMenu(products);
					DataManager::saveIngredients(ingredientTypes);
					DataManager::saveInventory(branchValues, ingredientTypes);
					statusVersion++;
					statusMessage = managementUi.getMessage();
				}
			} else if (currentUser) {
				if (CheckCollisionPointRec(mouse, {1140, 38, 112, 30})) {
					staffManagementUi.openChangePassModal();
				}
				for (int n = 0; n < navItems.size(); ++n) {
					Rectangle navRec = {20, 136.0f + static_cast<float>(n) * 52.0f, 180, 44};
					if (CheckCollisionPointRec(mouse, navRec)) {
						activeTab = navItems[n].tabId;
						statusVersion++;
						statusMessage = String("Đã mở: ") + navItems[n].label;
					}
				}
				if (CheckCollisionPointRec(mouse, {20, 618, 180, 42})) {
					currentUser = nullptr;
					activeTab = 0;
					loginDialog = true;
					usernameInput.clear();
					passwordInput.clear();
					statusVersion++;
					statusMessage = "Đã đăng xuất khỏi hệ thống";
				} else if (activeTab == 0) {
					if (posUi.handleClick(mouse, products, menuLookup, branchValues, orders, orderQueue, currentUser, statusMessage, statusVersion)) {
						DataManager::saveOrders(orders);
					}
				} else if (activeTab == 1) {
					reportUi.handleClick(mouse, orders, orderQueue, branchValues, *currentUser, statusMessage, statusVersion);
				} else if (activeTab == 2) {
					if (dutMapUi.handleClick(mouse, branchValues, chain, accounts, currentUser, statusMessage, statusVersion)) {
						DataManager::saveBranches(branchValues);
						DataManager::saveInventory(branchValues, ingredientTypes);
						DataManager::rebuildEmployeePool(accounts, employeePool, currentUser);
					}
				} else if (activeTab >= 3 && activeTab <= 5) {
					if (managementUi.handleClick(mouse, activeTab, *currentUser)) {
						statusVersion++;
						statusMessage = managementUi.getMessage();
						DataManager::saveMenu(products);
						DataManager::saveIngredients(ingredientTypes);
						DataManager::saveInventory(branchValues, ingredientTypes);
						DataManager::saveSchedules(scheduleService);
						DataManager::saveAttendance(timekeepingRecords);
					}
				} else if (activeTab == 6) {
					if (staffManagementUi.handleClick(mouse, accounts, branchValues, *currentUser, statusMessage, statusVersion)) {
						DataManager::rebuildEmployeePool(accounts, employeePool, currentUser);
					}
				}
			}
		}

		// Xử lý bàn phím
		if (loginDialog) {
			if (IsKeyPressed(KEY_BACKSPACE)) {
				if (loginField == 0 && !usernameInput.empty()) PopUtf8Char(usernameInput);
				if (loginField == 1 && !passwordInput.empty()) PopUtf8Char(passwordInput);
			}
			int keyChar = GetCharPressed();
			while (keyChar > 0) {
				String& input = loginField == 0 ? usernameInput : passwordInput;
				if (keyChar >= 32 && keyChar <= 126 && input.size() < 24) input += static_cast<char>(keyChar);
				keyChar = GetCharPressed();
			}
			if (IsKeyPressed(KEY_TAB)) loginField = loginField == 0 ? 1 : 0;
			if (IsKeyPressed(KEY_ENTER)) attemptLogin();
		} else if (staffManagementUi.isModalOpen()) {
			int keyPress = GetKeyPressed();
			int keyChar = GetCharPressed();
			if (staffManagementUi.handleInput(keyPress, keyChar, accounts, *currentUser, statusMessage, statusVersion)) {
				DataManager::rebuildEmployeePool(accounts, employeePool, currentUser);
			}
			while ((keyChar = GetCharPressed()) > 0) {
				if (staffManagementUi.handleInput(0, keyChar, accounts, *currentUser, statusMessage, statusVersion)) {
					DataManager::rebuildEmployeePool(accounts, employeePool, currentUser);
				}
			}
		} else if (dutMapUi.isModalOpen()) {
			int keyPress = GetKeyPressed();
			int keyChar = GetCharPressed();
			if (dutMapUi.handleInput(keyPress, keyChar, branchValues, chain, accounts, currentUser, statusMessage, statusVersion)) {
				DataManager::saveBranches(branchValues);
				DataManager::saveInventory(branchValues, ingredientTypes);
				DataManager::rebuildEmployeePool(accounts, employeePool, currentUser);
			}
			while ((keyChar = GetCharPressed()) > 0) {
				if (dutMapUi.handleInput(0, keyChar, branchValues, chain, accounts, currentUser, statusMessage, statusVersion)) {
					DataManager::saveBranches(branchValues);
					DataManager::saveInventory(branchValues, ingredientTypes);
					DataManager::rebuildEmployeePool(accounts, employeePool, currentUser);
				}
			}
		} else if (managementUi.isDialogOpen()) {
			int keyChar = GetCharPressed();
			int keyPress = GetKeyPressed();
			if (managementUi.updateInput(keyPress, keyChar)) {
				DataManager::saveMenu(products);
				DataManager::saveIngredients(ingredientTypes);
				DataManager::saveInventory(branchValues, ingredientTypes);
				statusVersion++;
				statusMessage = managementUi.getMessage();
			}
			while ((keyChar = GetCharPressed()) > 0) {
				if (managementUi.updateInput(0, keyChar)) {
					DataManager::saveMenu(products);
					DataManager::saveIngredients(ingredientTypes);
					DataManager::saveInventory(branchValues, ingredientTypes);
					statusVersion++;
					statusMessage = managementUi.getMessage();
				}
			}
		} else if (activeTab == 0 && posUi.isInputFocused()) {
			int keyPress = GetKeyPressed();
			int keyChar = GetCharPressed();
			posUi.handleInput(keyPress, keyChar);
			while ((keyChar = GetCharPressed()) > 0) {
				posUi.handleInput(0, keyChar);
			}
		} else if (currentUser && activeTab == 4) {
			int keyPress = GetKeyPressed();
			int keyChar = GetCharPressed();
			managementUi.updateInput(keyPress, keyChar);
			while ((keyChar = GetCharPressed()) > 0) {
				managementUi.updateInput(0, keyChar);
			}
		}

		// Cập nhật hiệu ứng chuyển cảnh & thông báo Toast
		if (statusVersion != lastStatusVersion) {
			lastStatusVersion = statusVersion;
			statusChangeTime = GetTime();
			statusType = Anim::Classify(statusMessage);
			if (!statusMessage.startsWith("Đã mở:")) {
				Anim::PushToast(statusMessage, statusType);
			}
			if (loginDialog && statusType == Anim::ToastType::Error) loginShakeTime = GetTime();
		}
		if (activeTab != lastActiveTab) {
			lastActiveTab = activeTab;
			tabChangeTime = GetTime();
		}
		if (loginDialog && !wasLoginDialog) loginOpenTime = GetTime();
		if (!loginDialog && wasLoginDialog) tabChangeTime = GetTime();
		wasLoginDialog = loginDialog;

		const Vector2 uiMouse = dialogConsumed ? Vector2{-10000.0f, -10000.0f} : mouse;
		const Color green{35, 105, 78, 255};
		const Color darkGreen{35, 55, 53, 255};
		const Color muted{90, 100, 94, 255};

		// VẼ GIAO DIỆN (DRAWING)
		BeginDrawing();
		BeginMode2D({{GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f}, {640, 380}, 0, scale});
		ClearBackground({245, 241, 232, 255});

		// Thanh tiêu đề trên cùng (Header Bar)
		DrawRectangle(0, 0, 1280, 88, {35, 55, 53, 255});
		DrawRectangle(0, 85, static_cast<int>(1280.0f * (0.96f + 0.04f * std::sin(animationTime * 2.0f))), 3, {248, 218, 154, 255});
		DrawUiText("BẾP NHỎ", 28, 16, 26, {248, 218, 154, 255});
		DrawUiText("HỆ THỐNG QUẢN LÝ BÁN HÀNG & CHUỖI NHÀ HÀNG", 28, 52, 13, {190, 208, 196, 255});

		DrawUiTextRight(dateLabel.c_str(), 1252, 16, 13, {224, 232, 222, 255});
		if (currentUser) {
			String userBadge = String(currentUser->getRoleTitle()) + ": " + currentUser->getName();
			DrawUiTextRight(userBadge.c_str(), 1125, 46, 13, {248, 218, 154, 255});
			Rectangle chgPassBtn = {1140, 38, 112, 30};
			Rectangle chgPassV = Anim::Button(chgPassBtn, Color{55, 78, 72, 255}, Color{70, 96, 90, 255}, uiMouse, true, 0.25f);
			DrawRectangleRoundedLines(chgPassV, 0.25f, 4, Color{248, 218, 154, 180});
			DrawUiTextInRect("ĐỔI MK", chgPassV, 12, Color{248, 218, 154, 255}, 0, 4.0f);
		} else {
			DrawUiTextRight("CHƯA ĐĂNG NHẬP", 1252, 46, 13, {210, 92, 68, 255});
		}

		// Thanh điều hướng bên trái (Sidebar)
		DrawRectangle(0, 88, 220, 672, {224, 231, 220, 255});
		DrawLine(220, 88, 220, 760, Color{208, 216, 205, 255});
		DrawUiText("DANH MỤC CHỨC NĂNG", 24, 108, 12, {85, 104, 95, 255});

		if (currentUser) {
			int activeIndex = -1;
			for (int n = 0; n < navItems.size(); ++n) {
				if (activeTab == navItems[n].tabId) activeIndex = n;
			}
			if (activeIndex >= 0) {
				float targetY = 136.0f + static_cast<float>(activeIndex) * 52.0f;
				navIndicatorY = navIndicatorY < 0.0f ? targetY : Anim::Smooth(navIndicatorY, targetY, 16.0f);
			}

			for (int n = 0; n < navItems.size(); ++n) {
				Rectangle navRec = {18, 136.0f + static_cast<float>(n) * 52.0f, 184, 44};
				bool isActive = (activeTab == navItems[n].tabId);
				if (CheckCollisionPointRec(uiMouse, navRec)) hoverHint = navItems[n].hint;
				if (!isActive) {
					Anim::Button(navRec, Color{224, 231, 220, 255}, Color{208, 220, 206, 255}, uiMouse, true, 0.22f, false);
				}
			}

			if (activeIndex >= 0) {
				Rectangle pill = {18, navIndicatorY, 184, 44};
				DrawRectangleRounded({pill.x + 1, pill.y + 3, pill.width, pill.height}, 0.22f, 6, Color{20, 40, 30, 40});
				DrawRectangleRounded(pill, 0.22f, 6, Color{35, 55, 53, 255});
				float barPulse = 0.5f + 0.5f * std::sin(animationTime * 3.0f);
				DrawRectangleRounded({pill.x + 6, pill.y + 10, 4, pill.height - 20}, 0.5f, 4,
					Anim::LerpColor(Color{248, 218, 154, 255}, Color{255, 236, 190, 255}, barPulse));
			}

			for (int n = 0; n < navItems.size(); ++n) {
				Rectangle navRec = {18, 136.0f + static_cast<float>(n) * 52.0f, 184, 44};
				bool isActive = (activeTab == navItems[n].tabId);
				float nudge = isActive ? 4.0f : 4.0f * Anim::HoverAmount(navRec);
				Rectangle textRec = {navRec.x + nudge, navRec.y, navRec.width - nudge, navRec.height};
				float overlap = 1.0f - clampVal(std::fabs(navIndicatorY - navRec.y) / 44.0f, 0.0f, 1.0f);
				Color txt = Anim::LerpColor(Color{32, 55, 47, 255}, RAYWHITE, isActive ? 1.0f : overlap);
				DrawUiTextInRect(navItems[n].label, textRec, 13, txt, -1, 14.0f);
			}

			Rectangle logoutRec = {18, 618, 184, 42};
			if (CheckCollisionPointRec(uiMouse, logoutRec)) hoverHint = "Đăng xuất tài khoản hiện tại";
			Rectangle lv = Anim::Button(logoutRec, Color{242, 228, 218, 255}, Color{250, 210, 198, 255}, uiMouse, true, 0.22f);
			DrawRectangleRoundedLines(lv, 0.22f, 4, Color{220, 175, 165, 255});
			DrawUiTextInRect("ĐĂNG XUẤT", lv, 13, Color{185, 55, 45, 255}, 0, 10.0f);
		}

		DrawRectangleRounded({16, 676, 188, 66}, 0.15f, 4, Color{214, 223, 210, 255});
		DrawUiText("TRẠNG THÁI HỆ THỐNG", 28, 688, 11, {65, 84, 75, 255});
		{
			float pulse = 0.5f + 0.5f * std::sin(animationTime * 3.2f);
			DrawCircleV({33, 719}, 6.0f + 3.0f * pulse, Anim::WithAlpha(Color{35, 135, 90, 255}, 0.25f * (1.0f - pulse)));
			DrawCircleV({33, 719}, 4.0f, Color{35, 135, 90, 255});
		}
		DrawUiTextFitted("Dữ liệu: Mã hoá (.dat)", 44, 712, 150, 12, {35, 105, 78, 255});

		// Nội dung Tab đang chọn
		float tabT = Anim::EaseOutCubic(static_cast<float>((GetTime() - tabChangeTime) / 0.32));
		rlPushMatrix();
		rlTranslatef(0.0f, (1.0f - tabT) * 16.0f, 0.0f);

		if (activeTab == 0) {
			posUi.draw(uiMouse, mouse, products, menuLookup, orderQueue, statusChangeTime);
		} else if (currentUser && activeTab == 1) {
			reportUi.draw(uiMouse, orders, orderQueue, branchValues, products, *currentUser);
		} else if (activeTab == 2) {
			dutMapUi.draw(uiMouse, branchValues, currentUser);
		} else if (currentUser && activeTab >= 3 && activeTab <= 5) {
			managementUi.draw(uiMouse, activeTab, *currentUser);
		} else if (currentUser && activeTab == 6) {
			staffManagementUi.draw(uiMouse, accounts, branchValues, *currentUser);
		}

		rlPopMatrix();

		// Thanh trạng thái dưới cùng (Status Bar)
		DrawRectangle(220, 722, 1060, 38, Color{236, 232, 222, 255});
		DrawLine(220, 722, 1280, 722, Color{218, 214, 204, 255});
		Anim::DrawStatusIcon({240, 741}, 7.0f, statusType, 1.0f);
		DrawUiTextFitted(statusMessage.c_str(), 255, 733, 1000, 13, Anim::ToastColor(statusType));

		// Hộp thoại Đăng nhập (Login Modal)
		if (loginDialog) {
			float loginAge = static_cast<float>(GetTime() - loginOpenTime);
			float fade = Anim::EaseOutCubic(clampVal(loginAge / 0.35f, 0.0f, 1.0f));
			float pop = Anim::EaseOutBack(clampVal(loginAge / 0.38f, 0.0f, 1.0f));

			DrawRectangle(0, 0, 1280, 760, Color{20, 30, 28, static_cast<unsigned char>(165.0f * fade)});

			float shakeX = 0.0f;
			double shakeAge = GetTime() - loginShakeTime;
			if (shakeAge < 0.45) {
				shakeX = static_cast<float>(std::sin(shakeAge * 45.0) * 8.0 * (1.0 - shakeAge / 0.45));
			}

			float cx = loginBoxRec.x + loginBoxRec.width * 0.5f;
			float cy = loginBoxRec.y + loginBoxRec.height * 0.5f;
			rlPushMatrix();
			rlTranslatef(cx + shakeX, cy, 0.0f);
			float s = 0.88f + 0.12f * pop;
			rlScalef(s, s, 1.0f);
			rlTranslatef(-cx, -cy, 0.0f);

			DrawRectangleRounded({loginBoxRec.x + 2, loginBoxRec.y + 8, loginBoxRec.width, loginBoxRec.height}, 0.05f, 8,
				Color{0, 0, 0, static_cast<unsigned char>(45.0f * fade)});
			DrawRectangleRounded(loginBoxRec, 0.05f, 8, {250, 247, 238, 255});
			DrawRectangleRoundedLines(loginBoxRec, 0.05f, 8, green);

			DrawUiTextFitted("HỆ THỐNG QUẢN LÝ BẾP NHỎ", 415, 222, 450, 21, darkGreen);
			DrawUiTextFitted("Đăng nhập tài khoản theo phân cấp quyền hạn (Employee / Manager / ChainManager)", 415, 254, 450, 12, muted);

			DrawUiText("Tên đăng nhập:", 415, 286, 13, {54, 66, 58, 255});
			DrawUiInputBox(loginUserBoxRec, usernameInput, "Nhập tên tài khoản...", loginField == 0, mouse);

			DrawUiText("Mật khẩu:", 415, 362, 13, {54, 66, 58, 255});
			DrawUiInputBox(loginPassBoxRec, passwordInput, "Nhập mật khẩu...", loginField == 1, mouse, true);

			Rectangle loginBtnV = Anim::Button(loginBtnRec, green, BrightenColor(green, 24), mouse, true, 0.2f);
			DrawUiTextInRect("ĐĂNG NHẬP HỆ THỐNG", loginBtnV, 14, RAYWHITE, 0, 12.0f);

			rlPopMatrix();
		}

		if (currentUser) {
			staffManagementUi.drawChangePassModal(mouse, *currentUser);
		}

		Anim::DrawToasts();

		if (!hoverHint.empty()) {
			float tooltipWidth = MeasureUiText(hoverHint.c_str(), 12) + 24.0f;
			float tooltipX = clampVal(mouse.x + 14.0f, 8.0f, 1272.0f - tooltipWidth);
			float tooltipY = maxVal(mouse.y - 34.0f, 92.0f);
			Rectangle tipRec = {tooltipX, tooltipY, tooltipWidth, 26};
			DrawRectangleRounded(tipRec, 0.25f, 4, {35, 55, 53, 245});
			DrawUiTextInRect(hoverHint.c_str(), tipRec, 12, RAYWHITE, 0, 8.0f);
		}

		EndMode2D();
		EndDrawing();
	}

	DataManager::clearEmployeePool(employeePool);
	UnloadUiFont();
	CloseWindow();
	return 0;
}
