#include "raylib.h"
#include "datastructures/BranchList.h"
#include "datastructures/MenuLookup.h"
#include "datastructures/OrderQueue.h"
#include "models/RestaurantChain.h"
#include "models/User.h"
#include "models/Timekeeping.h"
#include "services/ScheduleService.h"
#include "services/ShiftManager.h"
#include "UI/ManagementUI.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <functional>
#include <limits>
#include <string>
#include <vector>

struct Product {
	const char* name;
	const char* category;
	int price;
	Color color;
};

struct CartItem {
	int productIndex;
	int quantity;
};

static std::string encrypt(const std::string& value) {
	std::string result = value;
	for (char& character : result) character = static_cast<char>(character ^ 0x5A);
	return result;
}

static int cartTotal(const std::array<CartItem, 8>& cart);

static void saveEncryptedOrder(const Order& order) {
	std::ofstream file("orders.dat", std::ios::app | std::ios::binary);
	if (file) file << encrypt(std::to_string(order.id) + "|" + order.branch + "|" + std::to_string(order.total) + "\n");
}

static const char* roleName(Role role) {
	if (role == Role::Employee) return "Employee";
	if (role == Role::Manager) return "Manager";
	return "Branch Manager";
}

static bool canAccessTab(const User* user, int tab) {
	if (!user) return false;
	if (tab == 0) return user->getRole() == Role::Employee;
	if (tab == 4 || tab == 5 || tab == 3) return true;
	if (tab == 1) return user->getRole() != Role::Employee;
	if (tab == 2) return user->getRole() == Role::BranchManager;
	return false;
}

struct LoginAccount {
	User* user;
	std::string username;
	std::string password;
};

static void saveEncryptedAccount(const User& user, const std::string& password) {
	std::ofstream file("accounts.dat", std::ios::app | std::ios::binary);
	if (file) file << encrypt(user.username + "|" + std::to_string(static_cast<int>(user.role)) + "|" + password + "\n");
}

static std::string lowerText(std::string value) {
	for (char& character : value) character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
	return value;
}

static int promotionDiscount(const std::array<CartItem, 8>& cart, bool promotionEnabled) {
	if (!promotionEnabled) return 0;
	int subtotal = cartTotal(cart);
	bool hasMain = cart[1].quantity > 0 || cart[3].quantity > 0;
	bool hasDrink = cart[4].quantity > 0 || cart[5].quantity > 0 || cart[6].quantity > 0;
	if (hasMain && hasDrink) return std::min(15000, subtotal);
	return subtotal * 10 / 100;
}

struct GridPoint { int x; int y; };

struct MapRoute {
	std::vector<int> nodes;
	std::vector<int> explored;
	float distance = std::numeric_limits<float>::infinity();
	};

class MapGraph {
	int columns;
	int rows;
	std::vector<bool> blocked;

	int id(int x, int y) const { return y * columns + x; }
	bool inside(int x, int y) const { return x >= 0 && x < columns && y >= 0 && y < rows; }
	std::vector<int> neighbours(int node) const {
		int x = node % columns;
		int y = node / columns;
		std::vector<int> result;
		for (GridPoint direction : std::array<GridPoint, 4>{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}}) {
			int nextX = x + direction.x;
			int nextY = y + direction.y;
			if (inside(nextX, nextY) && !blocked[id(nextX, nextY)]) result.push_back(id(nextX, nextY));
		}
		return result;
	}

	MapRoute findRoute(int start, int target, bool aStar) const {
		const int nodeCount = columns * rows;
		std::vector<float> distance(nodeCount, std::numeric_limits<float>::infinity());
		std::vector<int> previous(nodeCount, -1);
		MapRoute route;
		using QueueEntry = std::pair<float, int>;
		std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> pending;
		distance[start] = 0.0f;
		pending.push({0.0f, start});
		while (!pending.empty()) {
			int current = pending.top().second;
			pending.pop();
			route.explored.push_back(current);
			if (current == target) break;
			for (int next : neighbours(current)) {
				float candidate = distance[current] + 1.0f;
				if (candidate >= distance[next]) continue;
				distance[next] = candidate;
				previous[next] = current;
				float priority = candidate;
				if (aStar) {
					int nextX = next % columns;
					int nextY = next / columns;
					priority += static_cast<float>(std::abs(nextX - (target % columns)) + std::abs(nextY - (target / columns)));
				}
				pending.push({priority, next});
			}
		}
		if (!std::isfinite(distance[target])) return route;
		route.distance = distance[target];
		for (int node = target; node != -1; node = previous[node]) route.nodes.push_back(node);
		std::reverse(route.nodes.begin(), route.nodes.end());
		return route;
	}

public:
	MapGraph(int width, int height) : columns(width), rows(height), blocked(width * height, false) {
		for (int y = 2; y < rows - 2; ++y) if (y != 7 && y != 14) blocked[id(12, y)] = true;
		for (int x = 4; x < columns - 4; ++x) if (x != 8 && x != 24) blocked[id(x, 10)] = true;
		for (int y = 4; y < 16; ++y) if (y != 12) blocked[id(21, y)] = true;
	}
	int width() const { return columns; }
	int height() const { return rows; }
	bool isBlocked(int x, int y) const { return blocked[id(x, y)]; }
	int nodeAt(int x, int y) const { return id(x, y); }
	MapRoute dijkstra(int start, int target) const { return findRoute(start, target, false); }
	MapRoute aStar(int start, int target) const { return findRoute(start, target, true); }
};

static constexpr std::array<Product, 8> products = {{
	{"Special Banh Mi", "Main dish", 35000, {235, 142, 72, 255}},
	{"Grilled Chicken Rice", "Main dish", 55000, {210, 92, 68, 255}},
	{"Beef Stir-fried Noodles", "Main dish", 48000, {224, 169, 74, 255}},
	{"Beef Pho", "Main dish", 60000, {82, 150, 128, 255}},
	{"Peach Lemongrass Tea", "Drink", 28000, {242, 126, 110, 255}},
	{"Vietnamese Milk Coffee", "Drink", 25000, {142, 101, 72, 255}},
	{"Orange Juice", "Drink", 30000, {236, 174, 61, 255}},
	{"Flan", "Dessert", 22000, {192, 122, 155, 255}}
}};

static int cartTotal(const std::array<CartItem, 8>& cart) {
	int total = 0;
	for (const CartItem& item : cart) {
		total += products[item.productIndex].price * item.quantity;
	}
	return total;
}

static std::string productCode(int productIndex) {
	return "M" + std::to_string(101 + productIndex);
}

static Font uiFont;

static void drawUiText(const char* text, int x, int y, int size, Color color) {
	Font font = uiFont.texture.id != 0 ? uiFont : GetFontDefault();
	float renderSize = static_cast<float>(size + (size <= 14 ? 6 : 4));
	DrawTextEx(font, text, {static_cast<float>(x), static_cast<float>(y)}, renderSize, 0.8f, color);
}

#define DrawText drawUiText

static void drawTextRight(const char* text, int right, int y, int size, Color color) {
	DrawText(text, right - MeasureText(text, size), y, size, color);
}

int main() {
	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 760, "Bep Nho - Quan ly ban hang");
	SetWindowMinSize(900, 600);
	uiFont = LoadFontEx("C:/Windows/Fonts/segoeuib.ttf", 48, nullptr, 0);
	SetTargetFPS(60);

	std::array<CartItem, 8> cart{};
	for (int i = 0; i < static_cast<int>(cart.size()); ++i) {
		cart[i] = {i, 0};
	}
	Employee employeeOne("EMP001", "employee1", "BR001");
	Employee employeeTwo("EMP002", "employee2", "BR002");
	Employee employeeThree("EMP003", "employee3", "BR003");
	Manager managerOne("MGR001", "manager1", "BR001");
	Manager managerTwo("MGR002", "manager2", "BR002");
	BranchManager branchManagerUser;
	employeeOne.showMenuOptions();
	employeeTwo.showMenuOptions();
	employeeThree.showMenuOptions();
	managerOne.showMenuOptions();
	managerTwo.showMenuOptions();
	branchManagerUser.showMenuOptions();
	std::vector<Employee*> employees{&employeeOne, &employeeTwo, &employeeThree, &managerOne, &managerTwo, &branchManagerUser};
	std::vector<Timekeeping> timekeepingRecords;
	for (Employee* user : employees) timekeepingRecords.emplace_back(user->getId());
	std::vector<LoginAccount> accounts{
		{&employeeOne, "employee1", "emp1pass"},
		{&employeeTwo, "employee2", "emp2pass"},
		{&employeeThree, "employee3", "emp3pass"},
		{&managerOne, "manager1", "mgr1pass"},
		{&managerTwo, "manager2", "mgr2pass"},
		{&branchManagerUser, "branchmanager", "branchpass"}
	};
	for (const LoginAccount& account : accounts)
		saveEncryptedAccount(*account.user, account.password);
	User* currentUser = nullptr;
	RestaurantChain chain("Small Kitchen");
	chain.addBranch("BR001","Central Branch",290.0f,285.0f);
	chain.addBranch("BR002","West Branch",470.0f,185.0f);
	chain.addBranch("BR003","East Branch",710.0f,330.0f);
	chain.addBranch("BR004","South Branch",560.0f,510.0f);
	std::vector<Branch> branchValues = chain.getBranches();
	int targetBranch = 2;
	MapGraph mapGraph(30, 20);
	std::array<GridPoint, 4> branchCells{{{2, 2}, {10, 2}, {26, 7}, {15, 17}}};
	float mapOriginX = 230.0f;
	float mapOriginY = 170.0f;
	float mapCellSize = 28.0f;
	int mapStart = mapGraph.nodeAt(branchCells[0].x, branchCells[0].y);
	int mapTarget = mapGraph.nodeAt(branchCells[targetBranch].x, branchCells[targetBranch].y);
	MapRoute dijkstraRoute = mapGraph.dijkstra(mapStart, mapTarget);
	MapRoute aStarRoute = mapGraph.aStar(mapStart, mapTarget);
	OrderQueue orderQueue;
	MenuLookup menuLookup;
	for (int i=0;i<static_cast<int>(products.size());++i) menuLookup.addItem({lowerText(productCode(i)),products[i].name,static_cast<float>(products[i].price)});
	ShiftManager shiftManager;
	ScheduleService scheduleService(&shiftManager);
	scheduleService.openOpenShift("OPEN1", "2026-10-09", "08:00", "16:00", 30, "BR001", 2);
	scheduleService.openOpenShift("OPEN2", "2026-10-10", "16:00", "23:00", 30, "BR002", 2);
	for (Branch& branch : branchValues) {
		branch.addStock("FLOUR", 50);
		branch.addStock("NOODLES", 50);
	}
	ManagementUI managementUi(chain, branchValues, scheduleService, shiftManager, timekeepingRecords, employees);

	std::string search;
	bool searchFocused = false;
	bool loginDialog = true;
	int loginField = 0;
	std::string usernameInput;
	std::string passwordInput;
	bool deliveryMode = true;
	bool useAStar = true;
	float pathAnimation = 0.0f;
	int activeTab = 0;
	bool discount = false;
	bool orderPlaced = false;
	int ordersToday = 24;
	int revenueToday = 4285000;
	int nextOrderId = 1001;
	float animationTime = 0.0f;
	std::string statusMessage = "Please log in";

	while (!WindowShouldClose()) {
		animationTime += GetFrameTime();
		bool compactLayout = GetScreenWidth() < 1150 || GetScreenHeight() < 680;
		mapOriginX = compactLayout ? 190.0f : 230.0f;
		mapOriginY = compactLayout ? 150.0f : 170.0f;
		mapCellSize = compactLayout ? 20.0f : 28.0f;
		if (activeTab == 2 && pathAnimation < 1000.0f) pathAnimation = std::min(1000.0f, pathAnimation + GetFrameTime() * 28.0f);
		float scale = std::min(GetScreenWidth() / 1280.0f, GetScreenHeight() / 760.0f);
		Vector2 mouse = GetMousePosition();
		mouse.x = (mouse.x - GetScreenWidth() / 2.0f) / scale + 640.0f;
		mouse.y = (mouse.y - GetScreenHeight() / 2.0f) / scale + 380.0f;
		std::string hoverHint;
		bool roleDialogConsumedInput = loginDialog;
		auto isHovered = [&](Rectangle bounds, const char* hint) {
			if (CheckCollisionPointRec(mouse, bounds)) {
				hoverHint = hint;
				return true;
			}
			return false;
		};
		bool hoverOrderTab = isHovered({28, 134, 180, 48}, "Open order creation");
		bool hoverStatsTab = canAccessTab(currentUser, 1) && isHovered({28, 190, 180, 48}, "View sales reports");
		bool hoverMapTab = canAccessTab(currentUser, 2) && isHovered({28, 246, 180, 48}, "Find the shortest delivery route");
		bool hoverRole = isHovered({28, 300, 180, 48}, "Switch role");
		bool hoverInventoryTab = canAccessTab(currentUser, 3) && isHovered({28, 356, 180, 48}, "Manage branch menu and inventory");
		bool hoverScheduleTab = canAccessTab(currentUser, 4) && isHovered({28, 412, 180, 48}, "Manage work schedules");
		bool hoverAttendanceTab = canAccessTab(currentUser, 5) && isHovered({28, 468, 180, 48}, "Record attendance and find branches");
		bool hoverDiscount = isHovered({960, 556, 250, 32}, "Toggle the promotion or combo");
		bool hoverPayment = isHovered({960, 606, 250, 42}, "Add the order to the queue and pay");
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			if (loginDialog) {
				roleDialogConsumedInput = true;
				if (CheckCollisionPointRec(mouse, {470, 330, 340, 46})) loginField = 0;
				if (CheckCollisionPointRec(mouse, {470, 435, 340, 46})) loginField = 1;
				if (CheckCollisionPointRec(mouse, {640, 500, 180, 40})) {
					for (const LoginAccount& account : accounts) {
						if (account.username == usernameInput && account.password == passwordInput) {
							currentUser = account.user;
							activeTab = canAccessTab(currentUser, 0) ? 0 : 1;
							loginDialog = false;
							usernameInput.clear();
							passwordInput.clear();
							statusMessage = "Login successful: " + std::string(roleName(currentUser->role));
							break;
						}
					}
					if (!currentUser) {
						statusMessage = "Invalid username or password";
						passwordInput.clear();
					}
				}
			} else {
			if (CheckCollisionPointRec(mouse, {28, 134, 180, 48}) && canAccessTab(currentUser, 0)) { activeTab = 0; statusMessage = "Orders opened"; }
			if (CheckCollisionPointRec(mouse, {28, 190, 180, 48}) && canAccessTab(currentUser, 1)) { activeTab = 1; statusMessage = "Sales report opened"; }
			if (CheckCollisionPointRec(mouse, {28, 246, 180, 48}) && canAccessTab(currentUser, 2)) { activeTab = 2; statusMessage = "Delivery map opened"; }
			if (CheckCollisionPointRec(mouse, {28, 356, 180, 48}) && canAccessTab(currentUser, 3)) { activeTab = 3; statusMessage = "Branch management opened"; }
			if (CheckCollisionPointRec(mouse, {28, 412, 180, 48}) && canAccessTab(currentUser, 4)) { activeTab = 4; statusMessage = "Schedules opened"; }
			if (CheckCollisionPointRec(mouse, {28, 468, 180, 48}) && canAccessTab(currentUser, 5)) { activeTab = 5; statusMessage = "Attendance opened"; }
			if (CheckCollisionPointRec(mouse, {28, 300, 180, 48})) {
				if (currentUser) {
					currentUser = nullptr;
					activeTab = 0;
					loginDialog = true;
					statusMessage = "Logged out";
				}
				else {
					loginDialog = true;
					roleDialogConsumedInput = true;
					passwordInput.clear();
				}
			}
			if (activeTab == 0) {
				if (CheckCollisionPointRec(mouse, {250, 190, 500, 40})) searchFocused = true;
				for (int i = 0; i < static_cast<int>(products.size()); ++i) {
					int column = i % 2;
					int row = i / 2;
					Rectangle addButton = {440.0f + column * 340.0f, 334.0f + row * 106.0f, 110.0f, 32.0f};
					isHovered(addButton, "Add item to order");
					if (CheckCollisionPointRec(mouse, addButton)) { cart[i].quantity++; statusMessage = "Added item: " + std::string(products[i].name); }
				}
				int visibleRow = 0;
				for (int i = 0; i < static_cast<int>(cart.size()); ++i) {
					if (cart[i].quantity == 0) continue;
					Rectangle removeButton = {960, 180.0f + visibleRow * 32.0f, 24, 24};
					if (CheckCollisionPointRec(mouse, removeButton)) { cart[i].quantity = std::max(0, cart[i].quantity - 1); statusMessage = "Item quantity decreased"; }
					visibleRow++;
				}
				if (CheckCollisionPointRec(mouse, {960, 606, 250, 42}) && cartTotal(cart) > 0) {
					orderPlaced = true;
					int paidTotal = cartTotal(cart) - promotionDiscount(cart, discount);
					Order order{nextOrderId,paidTotal,branchValues.front().getName(),"ORD"+std::to_string(nextOrderId),"KIOSK",{},"pending"};
					for (const CartItem& item : cart) if (item.quantity>0) order.items.push_back({productCode(item.productIndex),item.quantity});
					nextOrderId++;
					orderQueue.push(order);
					saveEncryptedOrder(order);
					statusMessage = "Order #" + std::to_string(order.id) + " added to queue";
					ordersToday++;
					revenueToday += paidTotal;
					cart = {};
					for (int i = 0; i < static_cast<int>(cart.size()); ++i) cart[i] = {i, 0};
				}
				if (CheckCollisionPointRec(mouse, {960, 556, 250, 32})) { discount = !discount; statusMessage = discount ? "Promotion enabled" : "Promotion disabled"; }
			}
			if (activeTab == 2) {
				bool branchClicked = false;
				for (int i = 0; i < static_cast<int>(branchValues.size()); ++i) {
					float branchX = mapOriginX + branchCells[i].x * mapCellSize + mapCellSize / 2.0f;
					float branchY = mapOriginY + branchCells[i].y * mapCellSize + mapCellSize / 2.0f;
					float dx = mouse.x - branchX;
					float dy = mouse.y - branchY;
					if (dx * dx + dy * dy <= 625.0f) {
						branchClicked = true;
						targetBranch = i;
						mapTarget = mapGraph.nodeAt(branchCells[targetBranch].x, branchCells[targetBranch].y);
						dijkstraRoute = mapGraph.dijkstra(mapStart, mapTarget);
						aStarRoute = mapGraph.aStar(mapStart, mapTarget);
						pathAnimation = 0.0f;
						statusMessage = "Selected " + branchValues[i].getName();
					}
				}
				int mapX = static_cast<int>((mouse.x - mapOriginX) / mapCellSize);
				int mapY = static_cast<int>((mouse.y - mapOriginY) / mapCellSize);
				if (!branchClicked && mapX >= 0 && mapX < mapGraph.width() && mapY >= 0 && mapY < mapGraph.height() && !mapGraph.isBlocked(mapX, mapY)) {
					mapTarget = mapGraph.nodeAt(mapX, mapY);
					targetBranch = -1;
					dijkstraRoute = mapGraph.dijkstra(mapStart, mapTarget);
					aStarRoute = mapGraph.aStar(mapStart, mapTarget);
					pathAnimation = 0.0f;
					statusMessage = "Selected a custom map point";
				}
				Rectangle algorithmButton = compactLayout ? Rectangle{830, 455, 170, 40} : Rectangle{1090, 455, 170, 40};
				if (CheckCollisionPointRec(mouse, algorithmButton)) { useAStar = !useAStar; pathAnimation = 0.0f; }
				Rectangle serviceButton = compactLayout ? Rectangle{830, 400, 170, 40} : Rectangle{1090, 400, 170, 40};
				if (CheckCollisionPointRec(mouse, serviceButton)) { deliveryMode = !deliveryMode; statusMessage = deliveryMode ? "Mode: delivery" : "Mode: dine-in"; }
			}
			if (activeTab >= 3 && managementUi.handleClick(mouse, activeTab, *currentUser)) {
				statusMessage = managementUi.getMessage();
			}
			}
		}
		if (activeTab == 0 && IsKeyPressed(KEY_A)) { cart[0].quantity++; statusMessage = "Key A: added Special Banh Mi"; }
		if (activeTab == 0 && IsKeyPressed(KEY_ENTER) && !cartTotal(cart)) { statusMessage = "The cart is empty"; }

		if (loginDialog) {
			if (IsKeyPressed(KEY_BACKSPACE)) {
				if (loginField == 0 && !usernameInput.empty()) usernameInput.pop_back();
				if (loginField == 1 && !passwordInput.empty()) passwordInput.pop_back();
			}
			int key = GetCharPressed();
			while (key > 0) {
				std::string& input = loginField == 0 ? usernameInput : passwordInput;
				if (key >= 32 && key <= 126 && input.size() < 24) input += static_cast<char>(key);
				key = GetCharPressed();
			}
			if (IsKeyPressed(KEY_TAB)) loginField = loginField == 0 ? 1 : 0;
			if (IsKeyPressed(KEY_ENTER)) {
				for (const LoginAccount& account : accounts) {
					if (account.username == usernameInput && account.password == passwordInput) {
						currentUser = account.user;
						activeTab = canAccessTab(currentUser, 0) ? 0 : 1;
						loginDialog = false;
						usernameInput.clear();
						passwordInput.clear();
						statusMessage = "Login successful: " + std::string(roleName(currentUser->role));
						break;
					}
				}
				if (!currentUser) {
					statusMessage = "Invalid username or password";
					passwordInput.clear();
				}
			}
		} else if (currentUser && activeTab == 4) {
			if (IsKeyPressed(KEY_BACKSPACE)) managementUi.updateInput(KEY_BACKSPACE);
			int key = GetCharPressed();
			while (key > 0) {
				managementUi.updateInput(key);
				key = GetCharPressed();
			}
		} else if (!roleDialogConsumedInput && activeTab == 0 && searchFocused && IsKeyPressed(KEY_BACKSPACE) && !search.empty()) search.pop_back();
		if (!loginDialog && !roleDialogConsumedInput && activeTab == 0 && searchFocused) {
			int key = GetCharPressed();
			while (key > 0) {
				if (key >= 32 && key <= 126 && search.size() < 24) search += static_cast<char>(key);
				key = GetCharPressed();
			}
			if (IsKeyPressed(KEY_ENTER)) {
				std::string query = lowerText(search);
				if (menuLookup.find(query) != nullptr) statusMessage = "Found product code: " + search;
				else statusMessage = "Filtering products by: " + search;
			}
		}
		if (IsKeyPressed(KEY_P) && currentUser && currentUser->canViewReports() && !orderQueue.empty()) { orderQueue.popNext(); statusMessage = "Processed the first queued order"; }

		BeginDrawing();
		BeginMode2D({{GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f}, {640, 380}, 0, scale});
		ClearBackground({245, 241, 232, 255});
		DrawRectangle(0, 0, 1280, 92, {35, 55, 53, 255});
		DrawRectangle(0, 89, static_cast<int>(1280.0f * (0.96f + 0.04f * std::sin(animationTime * 2.0f))), 3, {248, 218, 154, 255});
		DrawText("SMALL KITCHEN", 30, 20, 30, {248, 218, 154, 255});
		DrawText("SALES MANAGEMENT", 31, 56, 13, {190, 208, 196, 255});
		DrawText("Tuesday, 25/08/2026", 1050, 24, 16, {224, 232, 222, 255});
		DrawText(currentUser ? currentUser->username.c_str() : "LOGIN REQUIRED", 1050, 52, 13, {170, 190, 180, 255});
		DrawText(statusMessage.c_str(), 250, 715, 14, {35, 105, 78, 255});

		DrawRectangle(0, 92, 218, 668, {224, 231, 220, 255});
		DrawText("MENU", 30, 110, 14, {65, 84, 75, 255});
		if (currentUser) {
			DrawRectangle(28, 134, 180, 48, activeTab == 0 || hoverOrderTab ? Color{35, 55, 53, 255} : Color{224, 231, 220, 255});
			DrawText("ORDERS", 52, 150, 17, activeTab == 0 ? RAYWHITE : Color{32, 55, 47, 255});
			if (canAccessTab(currentUser, 1)) {
				DrawRectangle(28, 190, 180, 48, activeTab == 1 || hoverStatsTab ? Color{35, 55, 53, 255} : Color{224, 231, 220, 255});
				DrawText("REPORTS", 52, 206, 17, activeTab == 1 ? RAYWHITE : Color{32, 55, 47, 255});
			}
			if (canAccessTab(currentUser, 2)) {
				DrawRectangle(28, 246, 180, 48, activeTab == 2 || hoverMapTab ? Color{35, 55, 53, 255} : Color{224, 231, 220, 255});
				DrawText("DELIVERY MAP", 38, 261, 15, activeTab == 2 ? RAYWHITE : Color{32, 55, 47, 255});
			}
			if (canAccessTab(currentUser, 3)) {
				DrawRectangle(28, 356, 180, 48, activeTab == 3 || hoverInventoryTab ? Color{35, 55, 53, 255} : Color{224, 231, 220, 255});
				DrawText("INVENTORY", 48, 375, 14, activeTab == 3 ? RAYWHITE : Color{32, 55, 47, 255});
			}
			if (canAccessTab(currentUser, 4)) {
				DrawRectangle(28, 412, 180, 48, activeTab == 4 || hoverScheduleTab ? Color{35, 55, 53, 255} : Color{224, 231, 220, 255});
				DrawText("SCHEDULES", 48, 428, 14, activeTab == 4 ? RAYWHITE : Color{32, 55, 47, 255});
			}
			if (canAccessTab(currentUser, 5)) {
				DrawRectangle(28, 468, 180, 48, activeTab == 5 || hoverAttendanceTab ? Color{35, 55, 53, 255} : Color{224, 231, 220, 255});
				DrawText("ATTENDANCE", 43, 484, 13, activeTab == 5 ? RAYWHITE : Color{32, 55, 47, 255});
			}
			DrawRectangle(28, 300, 180, 48, hoverRole ? Color{255, 229, 177, 255} : Color{247, 239, 214, 255});
			DrawText("SWITCH ROLE", 52, 316, 15, {112, 76, 35, 255});
		}
		DrawText("HỆ THỐNG", 30, 690, 14, {65, 84, 75, 255});
		DrawText("Server: Connected", 30, 713, 14, {45, 108, 76, 255});

		if (activeTab == 0) {
			DrawText("Create Order", 250, 118, 28, {35, 55, 53, 255});
			DrawText("Choose items to add to the new order", 250, 155, 14, {104, 115, 108, 255});
			DrawRectangle(250, 190, 500, 40, RAYWHITE);
			DrawRectangleLines(250, 190, 500, 40, searchFocused ? Color{35, 105, 78, 255} : Color{202, 211, 201, 255});
			DrawText(search.empty() ? "Search products..." : search.c_str(), 268, 202, 15, search.empty() ? Color{150, 158, 151, 255} : Color{45, 56, 50, 255});
			DrawText("PRODUCTS", 250, 260, 12, {93, 112, 103, 255});

			for (int i = 0; i < static_cast<int>(products.size()); ++i) {
				const Product& product = products[i];
				std::string normalizedSearch = lowerText(search);
				if (!search.empty() && lowerText(product.name).find(normalizedSearch) == std::string::npos && lowerText(productCode(i)).find(normalizedSearch) == std::string::npos) continue;
				int column = i % 2;
				int row = i / 2;
				Rectangle card = {238.0f + column * 340.0f, 278.0f + row * 106.0f, 320, 92};
				DrawRectangleRec(card, RAYWHITE);
				DrawRectangle(238 + column * 340, 278 + row * 106, 7, 92, product.color);
				DrawText(product.name, card.x + 18, card.y + 15, 16, {42, 55, 48, 255});
				DrawText((productCode(i) + " - " + product.category).c_str(), card.x + 18, card.y + 40, 12, {126, 136, 127, 255});
				char price[32]; std::snprintf(price, sizeof(price), "%d d", product.price);
				DrawText(price, card.x + 18, card.y + 64, 14, {35, 105, 78, 255});
				Rectangle addButton = {card.x + 202, card.y + 56, 110, 32};
				bool hoverAdd = CheckCollisionPointRec(mouse, addButton);
				if (hoverAdd) hoverHint = "Add item to order";
				DrawRectangleRec(addButton, hoverAdd ? Color{48, 137, 103, 255} : Color{35, 105, 78, 255});
				DrawText("+ Add", card.x + 224, card.y + 65, 13, RAYWHITE);
			}

			DrawRectangle(930, 110, 300, 570, RAYWHITE);
			DrawText("CURRENT ORDER", 956, 136, 17, {35, 55, 53, 255});
			DrawLine(956, 164, 1204, 164, {213, 220, 211, 255});
			int visibleRow = 0;
			for (const CartItem& item : cart) {
				if (item.quantity == 0) continue;
				char line[96]; std::snprintf(line, sizeof(line), "%dx  %s", item.quantity, products[item.productIndex].name);
				DrawText(line, 956, 180 + visibleRow * 32, 13, {54, 66, 58, 255});
				DrawText("-", 968, 183 + visibleRow * 32, 16, {175, 86, 68, 255});
				DrawRectangleLines(960, 180 + visibleRow * 32, 24, 24, {210, 215, 207, 255});
				visibleRow++;
			}
			int total = cartTotal(cart);
			int finalTotal = total - promotionDiscount(cart, discount);
			DrawLine(956, 478, 1204, 478, {213, 220, 211, 255});
			char totalText[64]; std::snprintf(totalText, sizeof(totalText), "Subtotal: %d VND", total);
			DrawText(totalText, 956, 492, 14, {86, 98, 89, 255});
			if (discount) DrawText("Promotion", 1058, 492, 13, {177, 105, 54, 255});
			char finalText[64]; std::snprintf(finalText, sizeof(finalText), "%d VND", finalTotal);
			drawTextRight(finalText, 1204, 520, 24, {35, 105, 78, 255});
			DrawRectangle(960, 556, 250, 32, hoverDiscount ? Color{255, 229, 177, 255} : Color{247, 239, 214, 255});
			DrawText(discount ? "Remove promotion" : "Apply promotion", 1010, 566, 13, {130, 91, 48, 255});
			DrawRectangle(960, 606, 250, 42, total > 0 ? (hoverPayment ? Color{48, 137, 103, 255} : Color{35, 105, 78, 255}) : Color{172, 184, 173, 255});
			DrawText("PAY NOW", 1030, 619, 14, RAYWHITE);
			if (orderPlaced) DrawText("New order created", 1000, 658, 13, {58, 125, 84, 255});
		} else if (activeTab == 1) {
			DrawText("Today's Overview", 250, 118, 28, {35, 55, 53, 255});
			DrawText("Business performance for today", 250, 155, 14, {104, 115, 108, 255});
			DrawRectangle(250, 200, 280, 130, {35, 105, 78, 255});
			DrawText("DOANH THU", 274, 224, 13, {190, 220, 198, 255});
			char revenue[64]; std::snprintf(revenue, sizeof(revenue), "%d đ", revenueToday);
			DrawText(revenue, 274, 258, 26, RAYWHITE);
			DrawRectangle(560, 200, 280, 130, {177, 105, 54, 255});
			DrawText("ORDERS SOLD", 584, 224, 13, {244, 218, 185, 255});
			char orders[32]; std::snprintf(orders, sizeof(orders), "%d orders", ordersToday);
			DrawText(orders, 584, 258, 26, RAYWHITE);
			DrawText("TOP PRODUCTS", 250, 380, 13, {93, 112, 103, 255});
			DrawText("Grilled Chicken Rice", 270, 425, 18, {48, 64, 55, 255});
			DrawRectangle(520, 426, 350, 16, {208, 220, 207, 255});
			DrawRectangle(520, 426, 280, 16, {82, 150, 128, 255});
			DrawText("Beef Pho", 270, 475, 18, {48, 64, 55, 255});
			DrawRectangle(520, 476, 350, 16, {208, 220, 207, 255});
			DrawRectangle(520, 476, 230, 16, {235, 142, 72, 255});
			DrawText("Peach Lemongrass Tea", 270, 525, 18, {48, 64, 55, 255});
			DrawRectangle(520, 526, 350, 16, {208, 220, 207, 255});
			DrawRectangle(520, 526, 190, 16, {242, 126, 110, 255});
		} else if (activeTab == 2) {
			int panelX = compactLayout ? 830 : 1090;
			DrawText("Delivery Map", compactLayout ? 190 : 250, 118, 28, {35, 55, 53, 255});
			DrawText("XY grid graph 30 x 20 - select a branch or empty cell", compactLayout ? 190 : 230, 140, 14, {104, 115, 108, 255});
			DrawRectangle(static_cast<int>(mapOriginX), static_cast<int>(mapOriginY), 840, 560, {235, 231, 218, 255});
			for (int y = 0; y < mapGraph.height(); ++y) for (int x = 0; x < mapGraph.width(); ++x) {
				Rectangle cell = {mapOriginX + x * mapCellSize, mapOriginY + y * mapCellSize, mapCellSize, mapCellSize};
				DrawRectangleRec(cell, mapGraph.isBlocked(x, y) ? Color{104, 115, 108, 255} : Color{241, 238, 226, 255});
				DrawRectangleLines(static_cast<int>(cell.x), static_cast<int>(cell.y), static_cast<int>(mapCellSize), static_cast<int>(mapCellSize), {218, 214, 201, 255});
			}
			const MapRoute& selectedRoute = useAStar ? aStarRoute : dijkstraRoute;
			int exploredCount = std::min(static_cast<int>(selectedRoute.explored.size()), static_cast<int>(pathAnimation));
			for (int i = 0; i < exploredCount; ++i) {
				int node = selectedRoute.explored[i];
				int nodeX = node % mapGraph.width();
				int nodeY = node / mapGraph.width();
				DrawRectangle(static_cast<int>(mapOriginX + nodeX * mapCellSize + 8), static_cast<int>(mapOriginY + nodeY * mapCellSize + 8), 12, 12, {184, 202, 191, 255});
			}
			int routeStart = static_cast<int>(selectedRoute.explored.size());
			int visibleRouteNodes = pathAnimation >= routeStart
				? std::min(static_cast<int>(selectedRoute.nodes.size()), static_cast<int>(pathAnimation) - routeStart)
				: 0;
			for (int i = 0; i < visibleRouteNodes; ++i) {
				int node = selectedRoute.nodes[i];
				int nodeX = node % mapGraph.width();
				int nodeY = node / mapGraph.width();
				DrawRectangle(static_cast<int>(mapOriginX + nodeX * mapCellSize + 5), static_cast<int>(mapOriginY + nodeY * mapCellSize + 5), 18, 18, useAStar ? Color{235, 142, 72, 255} : Color{35, 105, 78, 255});
			}
			for (int i = 0; i < static_cast<int>(branchValues.size()); ++i) {
				float branchX = mapOriginX + branchCells[i].x * mapCellSize + mapCellSize / 2.0f;
				float branchY = mapOriginY + branchCells[i].y * mapCellSize + mapCellSize / 2.0f;
				DrawCircle(static_cast<int>(branchX), static_cast<int>(branchY), 11, i == targetBranch ? Color{210, 92, 68, 255} : Color{35, 55, 53, 255});
				DrawText(branchValues[i].getName().c_str(), static_cast<int>(branchX) - 48, static_cast<int>(branchY) + 15, 10, {48, 64, 55, 255});
			}
			DrawText(useAStar ? "THUẬT TOÁN A*" : "THUẬT TOÁN DIJKSTRA", panelX, 190, 13, {93, 112, 103, 255});
			std::string routeText = targetBranch >= 0 ? branchValues.front().getName() + " -> " + branchValues[targetBranch].getName() : "Central Branch -> custom point";
			DrawText(routeText.c_str(), panelX, 220, 17, {35, 105, 78, 255});
			char distanceText[80]; std::snprintf(distanceText, sizeof(distanceText), "Distance: %.0f cells | %d nodes", selectedRoute.distance, static_cast<int>(selectedRoute.nodes.size()));
			DrawText(distanceText, panelX, 252, 14, {54, 66, 58, 255});
			DrawText(deliveryMode ? "Recommendation: delivery" : "Recommendation: dine-in", panelX, 285, 15, {130, 91, 48, 255});
			DrawText(("Destination: " + (targetBranch >= 0 ? branchValues[targetBranch].getName() : "custom cell")).c_str(), panelX, 315, 14, {54, 66, 58, 255});
			char queueText[48]; std::snprintf(queueText, sizeof(queueText), "Queued orders: %d", orderQueue.size());
			DrawText(queueText, panelX, 345, 14, {54, 66, 58, 255});
			DrawText(currentUser && currentUser->canViewReports() ? "Press P to process next order" : "Employee order processing", panelX, 375, 13, {104, 115, 108, 255});
			DrawRectangle(panelX, 400, 170, 40, {247, 239, 214, 255});
			DrawText(deliveryMode ? "Delivery" : "Dine-in", panelX + 50, 412, 15, {130, 91, 48, 255});
			DrawRectangle(panelX, 455, 170, 40, {224, 231, 220, 255});
			DrawText(useAStar ? "A* / Dijkstra" : "Dijkstra / A*", panelX + 25, 467, 12, {54, 66, 58, 255});
		} else {
			managementUi.draw(mouse, activeTab, *currentUser);
		}
		if (loginDialog) {
			DrawRectangle(0, 0, 1280, 760, {20, 30, 28, 150});
			DrawRectangle(420, 230, 440, 320, {250, 247, 238, 255});
			DrawText("SIGN IN", 470, 270, 22, {35, 55, 53, 255});
			DrawText("Each account determines its own role", 470, 305, 14, {104, 115, 108, 255});
			DrawText("Username", 470, 340, 14, {54, 66, 58, 255});
			DrawRectangle(470, 355, 340, 46, RAYWHITE);
			DrawRectangleLines(470, 355, 340, 46, loginField == 0 ? Color{35, 105, 78, 255} : Color{202, 211, 201, 255});
			DrawText(usernameInput.empty() ? "Enter username" : usernameInput.c_str(), 486, 367, 17,
				usernameInput.empty() ? Color{150, 158, 151, 255} : Color{42, 55, 48, 255});
			DrawText("Password", 470, 420, 14, {54, 66, 58, 255});
			DrawRectangle(470, 435, 340, 46, RAYWHITE);
			DrawRectangleLines(470, 435, 340, 46, loginField == 1 ? Color{35, 105, 78, 255} : Color{202, 211, 201, 255});
			std::string masked(passwordInput.size(), '*');
			DrawText(masked.c_str(), 486, 447, 18, {42, 55, 48, 255});
			DrawText("Tab switches fields, Enter signs in", 470, 490, 13, {104, 115, 108, 255});
			DrawRectangle(640, 495, 180, 40, {224, 231, 220, 255});
			DrawText("SIGN IN", 690, 507, 14, {54, 66, 58, 255});
		}
		if (!hoverHint.empty()) {
			int tooltipWidth = MeasureText(hoverHint.c_str(), 14) + 24;
			int tooltipX = std::min(static_cast<int>(mouse.x) + 14, 1260 - tooltipWidth);
			int tooltipY = std::max(static_cast<int>(mouse.y) - 38, 96);
			DrawRectangle(tooltipX, tooltipY, tooltipWidth, 28, {35, 55, 53, 245});
			DrawText(hoverHint.c_str(), tooltipX + 12, tooltipY + 7, 14, RAYWHITE);
		}
		EndMode2D();
		EndDrawing();
	}

	CloseWindow();
	if (uiFont.texture.id != 0) UnloadFont(uiFont);
	return 0;
}
