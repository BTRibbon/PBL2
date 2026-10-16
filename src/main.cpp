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
#include <charconv>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <functional>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#ifdef _WIN32
extern "C" __declspec(dllimport) unsigned long __stdcall GetModuleFileNameA(
	void* module, char* filename, unsigned long size);
#endif

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

static int cartTotal(const std::array<CartItem, 8>& cart);

static std::vector<std::string> splitFields(const std::string& line, char delimiter = '|') {
	std::vector<std::string> fields;
	std::stringstream stream(line);
	std::string field;
	while (std::getline(stream, field, delimiter)) fields.push_back(field);
	return fields;
}

static bool parseInteger(const std::string& value, int& result) {
	const char* first = value.data();
	const char* last = first + value.size();
	const auto parsed = std::from_chars(first, last, result);
	return parsed.ec == std::errc{} && parsed.ptr == last;
}

static bool replaceTextFile(const std::string& path, const std::string& content) {
	const std::string temporaryPath = path + ".tmp";
	{
		std::ofstream file(temporaryPath, std::ios::trunc);
		if (!file) return false;
		file << content;
		if (!file.good()) return false;
	}
	std::remove(path.c_str());
	return std::rename(temporaryPath.c_str(), path.c_str()) == 0;
}

static bool loadOrders(std::vector<Order>& orders) {
	std::ifstream file("data/orders.txt");
	if (!file) return false;
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty()) continue;
		const std::vector<std::string> fields = splitFields(line);
		if (fields.size() != 7) return false;
		Order order{};
		if (!parseInteger(fields[0], order.id) || !parseInteger(fields[1], order.total)) return false;
		order.branch = fields[2];
		order.orderId = fields[3];
		order.customerId = fields[4];
		order.status = fields[5];
		for (const std::string& item : splitFields(fields[6], ',')) {
			const std::vector<std::string> itemFields = splitFields(item, ':');
			if (itemFields.size() != 2) return false;
			int quantity = 0;
			if (!parseInteger(itemFields[1], quantity) || quantity <= 0) return false;
			order.items.push_back({itemFields[0], quantity});
		}
		orders.push_back(order);
	}
	return true;
}

static bool saveOrders(const std::vector<Order>& orders) {
	std::ostringstream content;
	for (const Order& order : orders) {
		content << order.id << "|" << order.total << "|" << order.branch << "|"
			<< order.orderId << "|" << order.customerId << "|" << order.status << "|";
		for (std::size_t i = 0; i < order.items.size(); ++i) {
			if (i > 0) content << ",";
			content << order.items[i].first << ":" << order.items[i].second;
		}
		content << "\n";
	}
	return replaceTextFile("data/orders.txt", content.str());
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

struct AccountRecord {
	std::string id;
	std::string username;
	std::string name;
	std::string role;
	std::string branchId;
	std::string salt;
	std::string passwordHash;
	User* user = nullptr;
};

static std::string passwordHash(const std::string& password, const std::string& salt) {
	const std::string input = salt + ":" + password;
	std::uint64_t hash = 14695981039346656037ULL;
	for (unsigned char character : input) {
		hash ^= character;
		hash *= 1099511628211ULL;
	}
	std::ostringstream result;
	result << std::hex;
	result.width(16);
	result.fill('0');
	result << hash;
	return result.str();
}

static bool loadAccounts(std::vector<AccountRecord>& accounts) {
	std::ifstream file("data/accounts.dat", std::ios::binary);
	if (!file) return false;
	std::string encrypted((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	std::string plain;
	plain.reserve(encrypted.size());
	for (unsigned char character : encrypted) plain.push_back(static_cast<char>(character ^ 0x5A));

	std::istringstream input(plain);
	std::string line;
	while (std::getline(input, line)) {
		if (line.empty()) continue;
		std::vector<std::string> fields;
		std::stringstream lineStream(line);
		std::string field;
		while (std::getline(lineStream, field, '|')) fields.push_back(field);
		if (fields.size() != 7) return false;
		accounts.push_back({fields[0], fields[1], fields[2], fields[3], fields[4], fields[5], fields[6]});
	}
	return !accounts.empty();
}

static bool loadInventory(std::vector<Branch>& branchValues) {
	std::ifstream file("data/inventory.txt");
	if (!file) return false;
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty()) continue;
		const std::vector<std::string> fields = splitFields(line);
		if (fields.size() != 3) return false;
		int quantity = 0;
		if (!parseInteger(fields[2], quantity) || quantity < 0) return false;
		for (Branch& branch : branchValues)
			if (branch.getId() == fields[0]) branch.addStock(fields[1], quantity);
	}
	return true;
}

static bool saveInventory(const std::vector<Branch>& branches) {
	std::ostringstream content;
	for (const Branch& branch : branches) {
		content << branch.getId() << "|FLOUR|" << branch.getStock("FLOUR") << "\n";
		content << branch.getId() << "|NOODLES|" << branch.getStock("NOODLES") << "\n";
	}
	return replaceTextFile("data/inventory.txt", content.str());
}

static bool loadSchedules(ScheduleService& scheduleService) {
	std::ifstream file("data/schedules.txt");
	if (!file) return false;
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty()) continue;
		const std::vector<std::string> fields = splitFields(line);
		if (fields.size() != 7 && fields.size() != 8) return false;
		int breakMinutes = 0;
		int requiredEmployees = 0;
		if (!parseInteger(fields[4], breakMinutes) || !parseInteger(fields[6], requiredEmployees)) return false;
		auto schedule = scheduleService.openOpenShift(fields[0], fields[1], fields[2], fields[3],
			breakMinutes, fields[5], requiredEmployees);
		if (fields.size() == 8) for (const std::string& employeeId : splitFields(fields[7], ','))
			if (!employeeId.empty()) schedule->assignEmployee(employeeId);
	}
	return true;
}

static bool saveSchedules(const ScheduleService& scheduleService) {
	std::ostringstream content;
	for (const auto& schedule : scheduleService.getSchedules()) {
		content << schedule->getId() << "|" << schedule->getDate() << "|"
			<< schedule->getStartTime() << "|" << schedule->getEndTime() << "|"
			<< schedule->getBreakMinutes() << "|" << schedule->getBranchId() << "|"
			<< schedule->getRequiredEmployees() << "|";
		for (std::size_t i = 0; i < schedule->getEmployeeIds().size(); ++i) {
			if (i > 0) content << ",";
			content << schedule->getEmployeeIds()[i];
		}
		content << "\n";
	}
	return replaceTextFile("data/schedules.txt", content.str());
}

static bool loadAttendance(std::vector<Timekeeping>& records) {
	std::ifstream file("data/attendance.txt");
	if (!file) return false;
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty()) continue;
		const std::vector<std::string> fields = splitFields(line);
		if (fields.size() != 5) return false;
		for (Timekeeping& record : records) {
			if (record.getEmployeeId() == fields[1])
				record.addRecord({fields[0], fields[1], fields[2], fields[3], fields[4] == "1"});
		}
	}
	return true;
}

static bool saveAttendance(const std::vector<Timekeeping>& records) {
	std::ostringstream content;
	for (const Timekeeping& record : records)
		for (const AttendanceRecord& item : record.getRecords())
			content << item.scheduleId << "|" << item.employeeId << "|" << item.checkIn << "|"
				<< item.checkOut << "|" << (item.isPresent ? "1" : "0") << "\n";
	return replaceTextFile("data/attendance.txt", content.str());
}

static std::string currentDateLabel() {
	const std::time_t now = std::time(nullptr);
	std::tm localTime{};
#ifdef _WIN32
	localtime_s(&localTime, &now);
#else
	localtime_r(&now, &localTime);
#endif
	char buffer[32];
	std::strftime(buffer, sizeof(buffer), "%A, %d/%m/%Y", &localTime);
	return buffer;
}

static bool setWorkingDirectoryToExecutable(const char* executablePath) {
	(void)executablePath;
	std::filesystem::path executable;
#ifdef _WIN32
	char buffer[260];
	const unsigned long length = GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
	if (length == 0 || length == sizeof(buffer)) return false;
	executable = std::filesystem::path(buffer);
#else
	executable = std::filesystem::absolute(executablePath);
#endif
	const std::filesystem::path directory = executable.parent_path();
	if (directory.empty()) return false;
	std::error_code error;
	std::filesystem::current_path(directory, error);
	return !error;
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

static std::string formatCurrency(int value) {
	std::string digits = std::to_string(value);
	for (int position = static_cast<int>(digits.size()) - 3; position > 0; position -= 3)
		digits.insert(static_cast<std::size_t>(position), ".");
	return digits + " đ";
}

struct ReportStats {
	int revenue = 0;
	int orderCount = 0;
	int pendingCount = 0;
	std::array<int, products.size()> productQuantities{};
};

static ReportStats buildReportStats(const std::vector<Order>& orders, const User& user, const std::vector<Branch>& branches) {
	ReportStats stats;
	const bool allBranches = user.getRole() == Role::BranchManager;
	std::string branchName;
	if (!allBranches) {
		for (const Branch& branch : branches) {
			if (branch.getId() == user.getBranchId()) {
				branchName = branch.getName();
				break;
			}
		}
	}
	for (const Order& order : orders) {
		if (!allBranches && order.branch != branchName) continue;
		stats.revenue += order.total;
		stats.orderCount++;
		if (order.status == "pending") stats.pendingCount++;
		for (const auto& item : order.items) {
			for (int productIndex = 0; productIndex < static_cast<int>(products.size()); ++productIndex) {
				if (item.first == productCode(productIndex)) {
					stats.productQuantities[productIndex] += item.second;
					break;
				}
			}
		}
	}
	return stats;
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

static std::vector<int> vietnameseFontCodepoints() {
	std::vector<int> codepoints;
	for (int codepoint = 32; codepoint <= 126; ++codepoint) codepoints.push_back(codepoint);
	for (int codepoint = 160; codepoint <= 383; ++codepoint) codepoints.push_back(codepoint);
	for (int codepoint = 7840; codepoint <= 7929; ++codepoint) codepoints.push_back(codepoint);
	return codepoints;
}

int main(int argc, char** argv) {
	if (!setWorkingDirectoryToExecutable(argc > 0 ? argv[0] : "")) {
		std::fprintf(stderr, "Unable to locate the application directory.\n");
		return 1;
	}
	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 760, "Bep Nho - Quan ly ban hang");
	SetWindowMinSize(900, 600);
	const std::vector<int> fontCodepoints = vietnameseFontCodepoints();
	uiFont = LoadFontEx("C:/Windows/Fonts/segoeuib.ttf", 48, fontCodepoints.data(),
		static_cast<int>(fontCodepoints.size()));
	SetTargetFPS(60);

	std::array<CartItem, 8> cart{};
	for (int i = 0; i < static_cast<int>(cart.size()); ++i) {
		cart[i] = {i, 0};
	}
	std::vector<AccountRecord> accounts;
	if (!loadAccounts(accounts)) {
		std::fprintf(stderr, "Unable to load data/accounts.dat. Run build.ps1 first.\n");
		CloseWindow();
		return 1;
	}
	auto findAccount = [&](const std::string& username) -> AccountRecord* {
		for (AccountRecord& account : accounts)
			if (account.username == username) return &account;
		return nullptr;
	};
	AccountRecord* employeeOneData = findAccount("employee1");
	AccountRecord* employeeTwoData = findAccount("employee2");
	AccountRecord* employeeThreeData = findAccount("employee3");
	AccountRecord* managerOneData = findAccount("manager1");
	AccountRecord* managerTwoData = findAccount("manager2");
	AccountRecord* branchManagerData = findAccount("branchmanager");
	if (!employeeOneData || !employeeTwoData || !employeeThreeData ||
		!managerOneData || !managerTwoData || !branchManagerData) {
		std::fprintf(stderr, "Required account records are missing from data/accounts.dat.\n");
		CloseWindow();
		return 1;
	}
	Employee employeeOne(employeeOneData->id, employeeOneData->name, employeeOneData->branchId);
	Employee employeeTwo(employeeTwoData->id, employeeTwoData->name, employeeTwoData->branchId);
	Employee employeeThree(employeeThreeData->id, employeeThreeData->name, employeeThreeData->branchId);
	Manager managerOne(managerOneData->id, managerOneData->name, managerOneData->branchId);
	Manager managerTwo(managerTwoData->id, managerTwoData->name, managerTwoData->branchId);
	BranchManager branchManagerUser(branchManagerData->id, branchManagerData->name);
	employeeOne.setUsername(employeeOneData->username);
	employeeTwo.setUsername(employeeTwoData->username);
	employeeThree.setUsername(employeeThreeData->username);
	managerOne.setUsername(managerOneData->username);
	managerTwo.setUsername(managerTwoData->username);
	branchManagerUser.setUsername(branchManagerData->username);
	employeeOne.showMenuOptions();
	employeeTwo.showMenuOptions();
	employeeThree.showMenuOptions();
	managerOne.showMenuOptions();
	managerTwo.showMenuOptions();
	branchManagerUser.showMenuOptions();
	std::vector<Employee*> employees{&employeeOne, &employeeTwo, &employeeThree, &managerOne, &managerTwo, &branchManagerUser};
	std::vector<Timekeeping> timekeepingRecords;
	for (Employee* user : employees) timekeepingRecords.emplace_back(user->getId());
	if (!loadAttendance(timekeepingRecords)) {
		std::fprintf(stderr, "Unable to load data/attendance.txt.\n");
		CloseWindow();
		return 1;
	}
	employeeOneData->user = &employeeOne;
	employeeTwoData->user = &employeeTwo;
	employeeThreeData->user = &employeeThree;
	managerOneData->user = &managerOne;
	managerTwoData->user = &managerTwo;
	branchManagerData->user = &branchManagerUser;
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
	std::vector<Order> orders;
	if (!loadOrders(orders)) {
		std::fprintf(stderr, "Unable to load data/orders.txt.\n");
		CloseWindow();
		return 1;
	}
	for (const Order& order : orders)
		if (order.status == "pending") orderQueue.push(order);
	MenuLookup menuLookup;
	for (int i=0;i<static_cast<int>(products.size());++i) menuLookup.addItem({lowerText(productCode(i)),products[i].name,static_cast<float>(products[i].price)});
	ShiftManager shiftManager;
	ScheduleService scheduleService(&shiftManager);
	if (!loadSchedules(scheduleService)) {
		std::fprintf(stderr, "Unable to load data/schedules.txt.\n");
		CloseWindow();
		return 1;
	}
	if (!loadInventory(branchValues)) {
		std::fprintf(stderr, "Unable to load data/inventory.txt.\n");
		CloseWindow();
		return 1;
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
	int nextOrderId = 1001;
	for (const Order& order : orders) nextOrderId = std::max(nextOrderId, order.id + 1);
	const std::string dateLabel = currentDateLabel();
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
					for (AccountRecord& account : accounts) {
						if (account.username == usernameInput &&
							passwordHash(passwordInput, account.salt) == account.passwordHash) {
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
					std::string orderBranch = branchValues.front().getName();
					if (currentUser && currentUser->getRole() != Role::BranchManager) {
						for (const Branch& branch : branchValues) {
							if (branch.getId() == currentUser->getBranchId()) {
								orderBranch = branch.getName();
								break;
							}
						}
					}
					Order order{nextOrderId,paidTotal,orderBranch,"ORD"+std::to_string(nextOrderId),"KIOSK",{},"pending"};
					for (const CartItem& item : cart) if (item.quantity>0) order.items.push_back({productCode(item.productIndex),item.quantity});
					nextOrderId++;
					orderQueue.push(order);
					orders.push_back(order);
					if (saveOrders(orders))
						statusMessage = "Order #" + std::to_string(order.id) + " added to queue";
					else
						statusMessage = "Order created but could not be saved";
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
				if (!saveInventory(branchValues) ||
					!saveSchedules(scheduleService) ||
					!saveAttendance(timekeepingRecords)) {
					statusMessage = "Change applied but could not be saved";
				}
			}
			}
		}
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
				for (AccountRecord& account : accounts) {
					if (account.username == usernameInput &&
						passwordHash(passwordInput, account.salt) == account.passwordHash) {
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
		BeginDrawing();
		BeginMode2D({{GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f}, {640, 380}, 0, scale});
		ClearBackground({245, 241, 232, 255});
		DrawRectangle(0, 0, 1280, 92, {35, 55, 53, 255});
		DrawRectangle(0, 89, static_cast<int>(1280.0f * (0.96f + 0.04f * std::sin(animationTime * 2.0f))), 3, {248, 218, 154, 255});
		DrawText("SMALL KITCHEN", 30, 20, 30, {248, 218, 154, 255});
		DrawText("SALES MANAGEMENT", 31, 56, 13, {190, 208, 196, 255});
		DrawText(dateLabel.c_str(), 1050, 24, 16, {224, 232, 222, 255});
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
			const ReportStats report = buildReportStats(orders, *currentUser, branchValues);
			const int averageOrder = report.orderCount == 0 ? 0 : report.revenue / report.orderCount;
			DrawText("Báo cáo bán hàng", 250, 118, 28, {35, 55, 53, 255});
			DrawText(currentUser->getRole() == Role::BranchManager ? "Toàn bộ chi nhánh - phiên hiện tại" : "Chi nhánh của bạn - phiên hiện tại", 250, 155, 14, {104, 115, 108, 255});

			const std::array<Rectangle, 4> cards{{
				{250, 200, 220, 112}, {490, 200, 220, 112}, {730, 200, 220, 112}, {970, 200, 220, 112}
			}};
			const std::array<Color, 4> cardColors{{
				{35, 105, 78, 255}, {177, 105, 54, 255}, {73, 116, 132, 255}, {125, 91, 135, 255}
			}};
			const std::array<const char*, 4> cardLabels{{"DOANH THU", "TỔNG ĐƠN", "TRUNG BÌNH / ĐƠN", "ĐANG CHỜ"}};
			const std::array<std::string, 4> cardValues{{
				formatCurrency(report.revenue),
				std::to_string(report.orderCount) + " đơn",
				formatCurrency(averageOrder),
				std::to_string(report.pendingCount) + " đơn"
			}};
			for (int i = 0; i < 4; ++i) {
				DrawRectangleRec(cards[i], cardColors[i]);
				DrawText(cardLabels[i], static_cast<int>(cards[i].x + 18), static_cast<int>(cards[i].y + 22), 11, {235, 240, 232, 255});
				DrawText(cardValues[i].c_str(), static_cast<int>(cards[i].x + 18), static_cast<int>(cards[i].y + 60), i == 0 || i == 2 ? 17 : 22, RAYWHITE);
			}

			DrawText("SẢN PHẨM BÁN CHẠY", 250, 365, 13, {93, 112, 103, 255});
			std::array<int, products.size()> ranking{};
			for (int i = 0; i < static_cast<int>(products.size()); ++i) ranking[i] = i;
			std::sort(ranking.begin(), ranking.end(), [&](int left, int right) {
				return report.productQuantities[left] > report.productQuantities[right];
			});
			int maxQuantity = 0;
			for (int quantity : report.productQuantities) maxQuantity = std::max(maxQuantity, quantity);
			for (int row = 0; row < 5; ++row) {
				int productIndex = ranking[row];
				int quantity = report.productQuantities[productIndex];
				int y = 405 + row * 45;
				DrawText((std::to_string(row + 1) + ".").c_str(), 252, y, 14, {93, 112, 103, 255});
				DrawText(products[productIndex].name, 280, y, 15, {48, 64, 55, 255});
				DrawRectangle(560, y + 3, 360, 14, {208, 220, 207, 255});
				int barWidth = maxQuantity == 0 ? 0 : 360 * quantity / maxQuantity;
				DrawRectangle(560, y + 3, barWidth, 14, products[productIndex].color);
				DrawText((std::to_string(quantity) + " phần").c_str(), 940, y, 14, {48, 64, 55, 255});
			}
			DrawText("DỮ LIỆU ĐANG HIỂN THỊ", 250, 650, 12, {93, 112, 103, 255});
			DrawText("Tính từ các đơn hàng được tạo trong phiên hiện tại.", 250, 675, 13, {104, 115, 108, 255});
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
