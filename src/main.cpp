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
#include "UI/UIFont.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
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

namespace {
const Color darkGreen{35, 55, 53, 255};
const Color green{35, 105, 78, 255};
const Color muted{104, 115, 108, 255};
const Color panel{255, 255, 255, 255};
const Color panelAlt{248, 250, 248, 255};
const Color amber{180, 110, 40, 255};
}

struct CartItem {
	std::string productCode;
	int quantity;
};

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

static std::string lowerText(std::string value) {
	for (char& character : value) character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
	return value;
}

static std::string formatCurrency(int value) {
	std::string digits = std::to_string(value);
	for (int position = static_cast<int>(digits.size()) - 3; position > 0; position -= 3)
		digits.insert(static_cast<std::size_t>(position), ".");
	return digits + " đ";
}

static const Product* findProduct(const std::vector<Product>& products, const std::string& code) {
	for (const auto& prod : products) {
		if (prod.code == code) return &prod;
	}
	return nullptr;
}

static int cartTotal(const std::vector<CartItem>& cart, const std::vector<Product>& products) {
	int total = 0;
	for (const auto& item : cart) {
		const Product* p = findProduct(products, item.productCode);
		if (p) total += p->price * item.quantity;
	}
	return total;
}

static int computeDiscount(const std::vector<CartItem>& cart, const std::vector<Product>& products,
	int discountPercent, bool comboEnabled) {
	int subtotal = cartTotal(cart, products);
	if (subtotal <= 0) return 0;
	int discount = 0;
	if (discountPercent > 0) {
		discount += (subtotal * std::clamp(discountPercent, 0, 100)) / 100;
	}
	if (comboEnabled) {
		bool hasMain = false;
		bool hasDrink = false;
		for (const auto& item : cart) {
			const Product* p = findProduct(products, item.productCode);
			if (p) {
				if (p->category == "Món chính") hasMain = true;
				if (p->category == "Đồ uống") hasDrink = true;
			}
		}
		if (hasMain && hasDrink) discount += 15000;
	}
	return std::min(discount, subtotal);
}

static bool loadMenu(std::vector<Product>& products, MenuLookup& menuLookup) {
	std::ifstream file("data/menu.txt");
	if (!file) {
		products = {
			{"M101", "Bánh Mì Đặc Biệt", "Món chính", 35000, {210, 92, 68, 255}},
			{"M102", "Cơm Gà Nướng Mật Ong", "Món chính", 55000, {210, 92, 68, 255}},
			{"M103", "Mì Xào Bò Rau Cải", "Món chính", 48000, {210, 92, 68, 255}},
			{"M104", "Phở Bò Tái Nạm", "Món chính", 60000, {210, 92, 68, 255}},
			{"M105", "Trà Đào Cam Sả", "Đồ uống", 28000, {82, 150, 128, 255}},
			{"M106", "Cà Phê Sữa Đá", "Đồ uống", 25000, {82, 150, 128, 255}},
			{"M107", "Nước Cam Ép Tươi", "Đồ uống", 30000, {82, 150, 128, 255}},
			{"M108", "Bánh Flan Caramel", "Tráng miệng", 22000, {192, 122, 155, 255}}
		};
		for (const auto& p : products) menuLookup.addItem({p.code, p.name, static_cast<float>(p.price)});
		return true;
	}
	products.clear();
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty()) continue;
		std::vector<std::string> fields = splitFields(line);
		if (fields.size() < 4) continue;
		int price = 0;
		if (!parseInteger(fields[3], price)) continue;
		Color col = {235, 142, 72, 255};
		if (fields[2] == "Món chính") col = {210, 92, 68, 255};
		else if (fields[2] == "Đồ uống") col = {82, 150, 128, 255};
		else if (fields[2] == "Tráng miệng") col = {192, 122, 155, 255};
		products.push_back({fields[0], fields[1], fields[2], price, col});
		menuLookup.addItem({fields[0], fields[1], static_cast<float>(price)});
	}
	return !products.empty();
}

static bool saveMenu(const std::vector<Product>& products) {
	std::ostringstream content;
	for (const auto& p : products) {
		content << p.code << "|" << p.name << "|" << p.category << "|" << p.price << "\n";
	}
	return replaceTextFile("data/menu.txt", content.str());
}

static bool loadOrders(std::vector<Order>& orders) {
	std::ifstream file("data/orders.txt");
	if (!file) return false;
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty()) continue;
		const std::vector<std::string> fields = splitFields(line);
		if (fields.size() != 7) continue;
		Order order{};
		if (!parseInteger(fields[0], order.id) || !parseInteger(fields[1], order.total)) continue;
		order.branch = fields[2];
		order.orderId = fields[3];
		order.customerId = fields[4];
		order.status = fields[5];
		for (const std::string& item : splitFields(fields[6], ',')) {
			const std::vector<std::string> itemFields = splitFields(item, ':');
			if (itemFields.size() != 2) continue;
			int quantity = 0;
			if (!parseInteger(itemFields[1], quantity) || quantity <= 0) continue;
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
	if (role == Role::Employee) return "Nhân viên";
	if (role == Role::Manager) return "Quản lý chi nhánh";
	return "Giám đốc chuỗi";
}

static bool canAccessTab(const User* user, int tab) {
	if (!user) return false;
	if (tab == 0) return true;
	if (tab == 1) return user->getRole() != Role::Employee;
	if (tab == 2) return true;
	if (tab == 3) return true;
	if (tab == 4 || tab == 5) return true;
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
		if (fields.size() != 7) continue;
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
		if (fields.size() != 3) continue;
		int quantity = 0;
		if (!parseInteger(fields[2], quantity) || quantity < 0) continue;
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
		if (fields.size() != 7 && fields.size() != 8) continue;
		int breakMinutes = 0;
		int requiredEmployees = 0;
		if (!parseInteger(fields[4], breakMinutes) || !parseInteger(fields[6], requiredEmployees)) continue;
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
		if (fields.size() != 5) continue;
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
	const char* days[] = {"Chủ Nhật", "Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy"};
	char buffer[64];
	std::snprintf(buffer, sizeof(buffer), "%s, %02d/%02d/%04d",
		days[localTime.tm_wday], localTime.tm_mday, localTime.tm_mon + 1, localTime.tm_year + 1900);
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

static bool matchBranch(const std::string& orderBranch, const Branch& branch) {
	if (orderBranch == branch.getName() || orderBranch == branch.getId()) return true;
	if (branch.getId() == "BR001" && (orderBranch == "Central Branch" || orderBranch == "Chi nhánh Trung Tâm")) return true;
	if (branch.getId() == "BR002" && (orderBranch == "West Branch" || orderBranch == "Chi nhánh Phía Tây")) return true;
	if (branch.getId() == "BR003" && (orderBranch == "East Branch" || orderBranch == "Chi nhánh Phía Đông")) return true;
	if (branch.getId() == "BR004" && (orderBranch == "South Branch" || orderBranch == "Chi nhánh Phía Nam")) return true;
	return false;
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

struct BranchStat {
	std::string branchName;
	std::string branchId;
	int revenue = 0;
	int totalOrders = 0;
	int pendingOrders = 0;
};

struct ReportStats {
	int revenue = 0;
	int orderCount = 0;
	int pendingCount = 0;
	std::unordered_map<std::string, int> productQuantities;
	std::vector<BranchStat> branchBreakdown;
};

static ReportStats buildReportStats(const std::vector<Order>& orders, const User& user,
	const std::vector<Branch>& branches, int branchFilterIndex = -1) {
	ReportStats stats;
	const bool isBranchManager = user.getRole() == Role::BranchManager;

	std::string filteredBranchName = "";
	if (!isBranchManager) {
		for (const auto& b : branches) {
			if (b.getId() == user.getBranchId()) {
				filteredBranchName = b.getName();
				break;
			}
		}
	} else if (branchFilterIndex >= 0 && branchFilterIndex < static_cast<int>(branches.size())) {
		filteredBranchName = branches[branchFilterIndex].getName();
	}

	for (const auto& b : branches) {
		stats.branchBreakdown.push_back({b.getName(), b.getId(), 0, 0, 0});
	}

	for (const Order& order : orders) {
		for (auto& bs : stats.branchBreakdown) {
			for (const auto& b : branches) {
				if (b.getId() == bs.branchId && matchBranch(order.branch, b)) {
					bs.revenue += order.total;
					bs.totalOrders++;
					if (order.status == "pending") bs.pendingOrders++;
					break;
				}
			}
		}

		if (!filteredBranchName.empty()) {
			bool matches = false;
			for (const auto& b : branches) {
				if (b.getName() == filteredBranchName && matchBranch(order.branch, b)) {
					matches = true;
					break;
				}
			}
			if (!matches) continue;
		}

		stats.revenue += order.total;
		stats.orderCount++;
		if (order.status == "pending") stats.pendingCount++;

		for (const auto& item : order.items) {
			stats.productQuantities[item.first] += item.second;
		}
	}

	return stats;
}

int main(int argc, char** argv) {
	if (!setWorkingDirectoryToExecutable(argc > 0 ? argv[0] : "")) {
		std::fprintf(stderr, "Không thể tìm thư mục ứng dụng.\n");
		return 1;
	}

	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 760, "Bếp Nhỏ - Hệ Thống Quản Lý Bán Hàng");
	SetWindowMinSize(960, 620);
	InitUiFont();
	SetTargetFPS(60);

	std::vector<AccountRecord> accounts;
	if (!loadAccounts(accounts)) {
		std::fprintf(stderr, "Không thể tải data/accounts.dat. Hãy chạy build-macos.sh trước.\n");
		UnloadUiFont();
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
		std::fprintf(stderr, "Thiếu tài khoản mặc định trong hệ thống.\n");
		UnloadUiFont();
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

	std::vector<Employee*> employees{&employeeOne, &employeeTwo, &employeeThree, &managerOne, &managerTwo, &branchManagerUser};
	std::vector<Timekeeping> timekeepingRecords;
	for (Employee* user : employees) timekeepingRecords.emplace_back(user->getId());

	loadAttendance(timekeepingRecords);

	employeeOneData->user = &employeeOne;
	employeeTwoData->user = &employeeTwo;
	employeeThreeData->user = &employeeThree;
	managerOneData->user = &managerOne;
	managerTwoData->user = &managerTwo;
	branchManagerData->user = &branchManagerUser;

	User* currentUser = nullptr;

	RestaurantChain chain("Bếp Nhỏ");
	chain.addBranch("BR001", "Chi nhánh Trung Tâm", 290.0f, 285.0f);
	chain.addBranch("BR002", "Chi nhánh Phía Tây", 470.0f, 185.0f);
	chain.addBranch("BR003", "Chi nhánh Phía Đông", 710.0f, 330.0f);
	chain.addBranch("BR004", "Chi nhánh Phía Nam", 560.0f, 510.0f);

	std::vector<Branch> branchValues = chain.getBranches();

	int targetBranch = 2;
	MapGraph mapGraph(30, 20);
	std::array<GridPoint, 4> branchCells{{{2, 2}, {10, 2}, {26, 7}, {15, 17}}};
	const float mapOriginX = 245.0f;
	const float mapOriginY = 168.0f;
	const float mapCellSize = 24.0f;
	int mapStart = mapGraph.nodeAt(branchCells[0].x, branchCells[0].y);
	int mapTarget = mapGraph.nodeAt(branchCells[targetBranch].x, branchCells[targetBranch].y);
	MapRoute dijkstraRoute = mapGraph.dijkstra(mapStart, mapTarget);
	MapRoute aStarRoute = mapGraph.aStar(mapStart, mapTarget);

	OrderQueue orderQueue;
	std::vector<Order> orders;
	loadOrders(orders);
	for (const Order& order : orders)
		if (order.status == "pending") orderQueue.push(order);

	MenuLookup menuLookup;
	std::vector<Product> products;
	loadMenu(products, menuLookup);

	ShiftManager shiftManager;
	ScheduleService scheduleService(&shiftManager);
	loadSchedules(scheduleService);
	loadInventory(branchValues);

	ManagementUI managementUi(chain, branchValues, scheduleService, shiftManager,
		timekeepingRecords, employees, products, menuLookup);

	std::vector<CartItem> cart;
	std::string search;
	bool searchFocused = false;
	std::string categoryFilter = "Tất cả";

	bool loginDialog = true;
	int loginField = 0;
	std::string usernameInput;
	std::string passwordInput;

	bool deliveryMode = true;
	bool useAStar = true;
	float pathAnimation = 0.0f;
	int activeTab = 0;

	int discountPercent = 0;
	bool comboPromotion = false;
	std::string discountInputText = "0";
	bool discountInputFocused = false;

	int selectedReportBranch = -1;

	bool orderPlaced = false;
	int nextOrderId = 1001;
	for (const Order& order : orders) nextOrderId = std::max(nextOrderId, order.id + 1);

	const std::string dateLabel = currentDateLabel();
	float animationTime = 0.0f;
	std::string statusMessage = "Vui lòng đăng nhập để sử dụng hệ thống";

	// Shared layout rectangles for Login Modal
	const Rectangle loginBoxRec = {380, 155, 520, 435};
	const Rectangle loginUserBoxRec = {415, 275, 450, 42};
	const Rectangle loginPassBoxRec = {415, 355, 450, 42};
	const Rectangle loginBtnRec = {415, 442, 450, 44};

	while (!WindowShouldClose()) {
		animationTime += GetFrameTime();

		if (activeTab == 2 && pathAnimation < 1000.0f) {
			pathAnimation = std::min(1000.0f, pathAnimation + GetFrameTime() * 28.0f);
		}

		float scale = std::min(GetScreenWidth() / 1280.0f, GetScreenHeight() / 760.0f);
		Vector2 mouse = GetMousePosition();
		mouse.x = (mouse.x - GetScreenWidth() / 2.0f) / scale + 640.0f;
		mouse.y = (mouse.y - GetScreenHeight() / 2.0f) / scale + 380.0f;

		std::string hoverHint;
		bool dialogConsumed = loginDialog || managementUi.isDialogOpen();

		auto isHovered = [&](Rectangle bounds, const char* hint) {
			if (CheckCollisionPointRec(mouse, bounds)) {
				hoverHint = hint;
				return true;
			}
			return false;
		};

		bool hoverOrderTab = isHovered({20, 134, 180, 44}, "Màn hình bán hàng & Tạo đơn");
		bool hoverStatsTab = canAccessTab(currentUser, 1) && isHovered({20, 188, 180, 44}, "Báo cáo doanh thu & Thống kê");
		bool hoverMapTab = isHovered({20, 242, 180, 44}, "Bản đồ giao hàng & Tìm đường");
		bool hoverInventoryTab = isHovered({20, 296, 180, 44}, "Quản lý thực đơn & Kho nguyên liệu");
		bool hoverScheduleTab = isHovered({20, 350, 180, 44}, "Lịch làm việc & Phân ca");
		bool hoverAttendanceTab = isHovered({20, 404, 180, 44}, "Điểm danh & Chấm công");
		bool hoverLogout = isHovered({20, 470, 180, 42}, "Đăng xuất tài khoản");

		// MOUSE CLICK HANDLING
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			if (loginDialog) {
				if (CheckCollisionPointRec(mouse, loginUserBoxRec)) loginField = 0;
				if (CheckCollisionPointRec(mouse, loginPassBoxRec)) loginField = 1;
				if (CheckCollisionPointRec(mouse, loginBtnRec)) {
					for (AccountRecord& account : accounts) {
						if (account.username == usernameInput &&
							passwordHash(passwordInput, account.salt) == account.passwordHash) {
							currentUser = account.user;
							activeTab = canAccessTab(currentUser, 0) ? 0 : 1;
							loginDialog = false;
							usernameInput.clear();
							passwordInput.clear();
							statusMessage = "Đăng nhập thành công! Vai trò: " + std::string(roleName(currentUser->role));
							break;
						}
					}
					if (!currentUser) {
						statusMessage = "Sai tên đăng nhập hoặc mật khẩu!";
						passwordInput.clear();
					}
				}
			} else if (managementUi.isDialogOpen()) {
				if (managementUi.handleClick(mouse, activeTab, *currentUser)) {
					saveMenu(products);
					statusMessage = managementUi.getMessage();
				}
			} else {
				// Sidebar tab switching
				if (CheckCollisionPointRec(mouse, {20, 134, 180, 44})) { activeTab = 0; statusMessage = "Đã mở màn hình Bán hàng"; }
				if (canAccessTab(currentUser, 1) && CheckCollisionPointRec(mouse, {20, 188, 180, 44})) { activeTab = 1; statusMessage = "Đã mở Báo cáo bán hàng"; }
				if (CheckCollisionPointRec(mouse, {20, 242, 180, 44})) { activeTab = 2; statusMessage = "Đã mở Bản đồ giao hàng"; }
				if (CheckCollisionPointRec(mouse, {20, 296, 180, 44})) { activeTab = 3; statusMessage = "Đã mở Quản lý Kho & Thực đơn"; }
				if (CheckCollisionPointRec(mouse, {20, 350, 180, 44})) { activeTab = 4; statusMessage = "Đã mở Lịch làm việc"; }
				if (CheckCollisionPointRec(mouse, {20, 404, 180, 44})) { activeTab = 5; statusMessage = "Đã mở Điểm danh"; }

				// Logout button
				if (CheckCollisionPointRec(mouse, {20, 470, 180, 42})) {
					currentUser = nullptr;
					activeTab = 0;
					loginDialog = true;
					usernameInput.clear();
					passwordInput.clear();
					statusMessage = "Đã đăng xuất khỏi hệ thống";
				}

				// TAB 0: POS & ORDERING
				if (activeTab == 0) {
					searchFocused = CheckCollisionPointRec(mouse, {245, 148, 305, 36});

					const char* filterCategories[] = {"Tất cả", "Món chính", "Đồ uống", "Tráng miệng"};
					for (int c = 0; c < 4; ++c) {
						Rectangle catBtn = {562.0f + c * 90.0f, 148.0f, 84.0f, 36.0f};
						if (CheckCollisionPointRec(mouse, catBtn)) {
							categoryFilter = filterCategories[c];
						}
					}

					int visibleIndex = 0;
					for (int i = 0; i < static_cast<int>(products.size()); ++i) {
						const Product& product = products[i];
						if (categoryFilter != "Tất cả" && product.category != categoryFilter) continue;
						std::string normalizedSearch = lowerText(search);
						if (!search.empty() && lowerText(product.name).find(normalizedSearch) == std::string::npos &&
							lowerText(product.code).find(normalizedSearch) == std::string::npos) continue;

						int column = visibleIndex % 2;
						int row = visibleIndex / 2;
						if (row >= 4) break;

						Rectangle card = {245.0f + column * 340.0f, 200.0f + row * 106.0f, 325, 94};
						Rectangle addBtn = {card.x + 210, card.y + 54, 100, 30};

						if (CheckCollisionPointRec(mouse, addBtn) || CheckCollisionPointRec(mouse, card)) {
							orderPlaced = false;
							bool found = false;
							for (auto& item : cart) {
								if (item.productCode == product.code) {
									item.quantity++;
									found = true;
									break;
								}
							}
							if (!found) cart.push_back({product.code, 1});
							statusMessage = "Đã thêm món: " + product.name;
						}
						visibleIndex++;
					}

					// Cart item row buttons
					int cartRow = 0;
					for (int i = 0; i < static_cast<int>(cart.size()); ++i) {
						if (cartRow >= 8) break;
						float rowY = 162.0f + cartRow * 34.0f;
						Rectangle minusBtn = {1190, rowY, 22, 22};
						Rectangle plusBtn = {1216, rowY, 22, 22};
						if (CheckCollisionPointRec(mouse, minusBtn)) {
							cart[i].quantity--;
							if (cart[i].quantity <= 0) {
								cart.erase(cart.begin() + i);
								--i;
							}
							break;
						}
						if (CheckCollisionPointRec(mouse, plusBtn)) {
							cart[i].quantity++;
							break;
						}
						cartRow++;
					}

					// Clear cart button
					if (!cart.empty() && CheckCollisionPointRec(mouse, {1175, 116, 66, 26})) {
						cart.clear();
						statusMessage = "Đã xoá toàn bộ giỏ hàng";
					}

					// Discount % preset buttons
					int presets[] = {0, 10, 20, 30, 50};
					for (int p = 0; p < 5; ++p) {
						Rectangle pBtn = {952.0f + p * 58.0f, 478.0f, 52.0f, 26.0f};
						if (CheckCollisionPointRec(mouse, pBtn)) {
							discountPercent = presets[p];
							discountInputText = std::to_string(discountPercent);
							discountInputFocused = false;
						}
					}

					// Custom discount input box
					Rectangle discBox = {1158, 510, 80, 26};
					discountInputFocused = CheckCollisionPointRec(mouse, discBox);

					// Combo promotion toggle button
					Rectangle comboBtn = {952, 544, 286, 28};
					if (CheckCollisionPointRec(mouse, comboBtn)) {
						comboPromotion = !comboPromotion;
						statusMessage = comboPromotion ? "Đã bật ưu đãi combo (-15.000 đ)" : "Đã tắt ưu đãi combo";
					}

					// Pay / Checkout button
					int totalAmount = cartTotal(cart, products);
					int discAmount = computeDiscount(cart, products, discountPercent, comboPromotion);
					int finalPaid = std::max(0, totalAmount - discAmount);

					if (CheckCollisionPointRec(mouse, {952, 634, 286, 42}) && totalAmount > 0) {
						orderPlaced = true;
						std::string orderBranch = branchValues.front().getName();
						if (currentUser && currentUser->getRole() != Role::BranchManager) {
							for (const Branch& branch : branchValues) {
								if (branch.getId() == currentUser->getBranchId()) {
									orderBranch = branch.getName();
									break;
								}
							}
						}
						Order order{nextOrderId, finalPaid, orderBranch, "ORD" + std::to_string(nextOrderId), "KIOSK", {}, "pending"};
						for (const auto& item : cart) {
							order.items.push_back({item.productCode, item.quantity});
						}
						nextOrderId++;
						orderQueue.push(order);
						orders.push_back(order);
						if (saveOrders(orders)) {
							statusMessage = "Đã thanh toán thành công đơn hàng #" + order.orderId + " (" + formatCurrency(finalPaid) + ")!";
						} else {
							statusMessage = "Đã tạo đơn nhưng không thể lưu file orders.txt";
						}
						cart.clear();
					}
				}

				// TAB 1: REPORTS & ANALYTICS
				if (activeTab == 1 && currentUser && currentUser->getRole() == Role::BranchManager) {
					Rectangle allBtn = {310, 138, 115, 28};
					if (CheckCollisionPointRec(mouse, allBtn)) {
						selectedReportBranch = -1;
					}
					for (int b = 0; b < static_cast<int>(branchValues.size()); ++b) {
						Rectangle bBtn = {433.0f + b * 204.0f, 138.0f, 196.0f, 28.0f};
						if (CheckCollisionPointRec(mouse, bBtn)) {
							selectedReportBranch = b;
						}
					}
				}

				// TAB 2: DELIVERY MAP
				if (activeTab == 2) {
					bool branchClicked = false;
					for (int i = 0; i < static_cast<int>(branchValues.size()); ++i) {
						float branchX = mapOriginX + branchCells[i].x * mapCellSize + mapCellSize / 2.0f;
						float branchY = mapOriginY + branchCells[i].y * mapCellSize + mapCellSize / 2.0f;
						float dx = mouse.x - branchX;
						float dy = mouse.y - branchY;
						if (dx * dx + dy * dy <= 400.0f) {
							branchClicked = true;
							targetBranch = i;
							mapTarget = mapGraph.nodeAt(branchCells[targetBranch].x, branchCells[targetBranch].y);
							dijkstraRoute = mapGraph.dijkstra(mapStart, mapTarget);
							aStarRoute = mapGraph.aStar(mapStart, mapTarget);
							pathAnimation = 0.0f;
							statusMessage = "Đã chọn chi nhánh: " + branchValues[i].getName();
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
						statusMessage = "Đã chọn điểm giao hàng tự do (" + std::to_string(mapX) + ", " + std::to_string(mapY) + ")";
					}

					Rectangle serviceBtn = {995, 425, 245, 36};
					if (CheckCollisionPointRec(mouse, serviceBtn)) {
						deliveryMode = !deliveryMode;
						statusMessage = deliveryMode ? "Chế độ: Giao hàng tận nơi" : "Chế độ: Phục vụ tại bàn";
					}
					Rectangle algoBtn = {995, 470, 245, 36};
					if (CheckCollisionPointRec(mouse, algoBtn)) {
						useAStar = !useAStar;
						pathAnimation = 0.0f;
						statusMessage = useAStar ? "Đã chuyển sang thuật toán A*" : "Đã chuyển sang thuật toán Dijkstra";
					}
					Rectangle processBtn = {995, 515, 245, 40};
					if (CheckCollisionPointRec(mouse, processBtn)) {
						if (!orderQueue.empty()) {
							Order nextOrd = orderQueue.popNext();
							for (auto& ord : orders) {
								if (ord.id == nextOrd.id) {
									ord.status = "completed";
									break;
								}
							}
							saveOrders(orders);
							statusMessage = "Đã hoàn thành đơn hàng #" + nextOrd.orderId + " (" + nextOrd.branch + ")";
						} else {
							statusMessage = "Hàng đợi đơn hàng đang trống!";
						}
					}
				}

				// TABS 3, 4, 5: MANAGEMENT UI
				if (activeTab >= 3 && managementUi.handleClick(mouse, activeTab, *currentUser)) {
					statusMessage = managementUi.getMessage();
					saveMenu(products);
					saveInventory(branchValues);
					saveSchedules(scheduleService);
					saveAttendance(timekeepingRecords);
				}
			}
		}

		// KEYBOARD HANDLING
		if (activeTab == 2 && IsKeyPressed(KEY_P) && !dialogConsumed) {
			if (!orderQueue.empty()) {
				Order nextOrd = orderQueue.popNext();
				for (auto& ord : orders) {
					if (ord.id == nextOrd.id) {
						ord.status = "completed";
						break;
					}
				}
				saveOrders(orders);
				statusMessage = "Đã hoàn tất xử lý đơn #" + nextOrd.orderId + " (" + nextOrd.branch + ")";
			} else {
				statusMessage = "Hàng đợi rỗng!";
			}
		}

		if (loginDialog) {
			if (IsKeyPressed(KEY_BACKSPACE)) {
				if (loginField == 0 && !usernameInput.empty()) PopUtf8Char(usernameInput);
				if (loginField == 1 && !passwordInput.empty()) PopUtf8Char(passwordInput);
			}
			int keyChar = GetCharPressed();
			while (keyChar > 0) {
				std::string& input = loginField == 0 ? usernameInput : passwordInput;
				if (keyChar >= 32 && keyChar <= 126 && input.size() < 24) input += static_cast<char>(keyChar);
				keyChar = GetCharPressed();
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
						statusMessage = "Đăng nhập thành công! Chức vụ: " + std::string(roleName(currentUser->role));
						break;
					}
				}
				if (!currentUser) {
					statusMessage = "Sai tên đăng nhập hoặc mật khẩu!";
					passwordInput.clear();
				}
			}
		} else if (managementUi.isDialogOpen()) {
			int keyChar = GetCharPressed();
			int keyPress = GetKeyPressed();
			managementUi.updateInput(keyPress, keyChar);
			while ((keyChar = GetCharPressed()) > 0) {
				managementUi.updateInput(0, keyChar);
			}
		} else if (activeTab == 0 && discountInputFocused) {
			if (IsKeyPressed(KEY_BACKSPACE) && !discountInputText.empty()) {
				discountInputText.pop_back();
				discountPercent = discountInputText.empty() ? 0 : std::clamp(std::stoi(discountInputText), 0, 100);
			}
			int keyChar = GetCharPressed();
			while (keyChar > 0) {
				if (std::isdigit(keyChar) && discountInputText.size() < 3) {
					if (discountInputText == "0") discountInputText.clear();
					discountInputText += static_cast<char>(keyChar);
					try {
						discountPercent = std::clamp(std::stoi(discountInputText), 0, 100);
						discountInputText = std::to_string(discountPercent);
					} catch (...) {}
				}
				keyChar = GetCharPressed();
			}
		} else if (activeTab == 0 && searchFocused) {
			if (IsKeyPressed(KEY_BACKSPACE) && !search.empty()) PopUtf8Char(search);
			int keyChar = GetCharPressed();
			while (keyChar > 0) {
				AppendUtf8Codepoint(search, keyChar, 30);
				keyChar = GetCharPressed();
			}
		} else if (currentUser && activeTab == 4) {
			int keyPress = GetKeyPressed();
			int keyChar = GetCharPressed();
			managementUi.updateInput(keyPress, keyChar);
			while ((keyChar = GetCharPressed()) > 0) {
				managementUi.updateInput(0, keyChar);
			}
		}

		// DRAWING
		BeginDrawing();
		BeginMode2D({{GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f}, {640, 380}, 0, scale});
		ClearBackground({245, 241, 232, 255});

		// Header Bar (y = 0..88)
		DrawRectangle(0, 0, 1280, 88, {35, 55, 53, 255});
		DrawRectangle(0, 85, static_cast<int>(1280.0f * (0.96f + 0.04f * std::sin(animationTime * 2.0f))), 3, {248, 218, 154, 255});
		DrawUiText("BẾP NHỎ", 28, 16, 26, {248, 218, 154, 255});
		DrawUiText("HỆ THỐNG QUẢN LÝ BÁN HÀNG & CHUỖI NHÀ HÀNG", 28, 52, 13, {190, 208, 196, 255});

		DrawUiTextRight(dateLabel.c_str(), 1252, 20, 14, {224, 232, 222, 255});
		if (currentUser) {
			std::string userBadge = std::string(roleName(currentUser->role)) + ": " + currentUser->getName();
			DrawUiTextRight(userBadge.c_str(), 1252, 48, 13, {248, 218, 154, 255});
		} else {
			DrawUiTextRight("CHƯA ĐĂNG NHẬP", 1252, 48, 13, {210, 92, 68, 255});
		}

		// Left Navigation Sidebar (x = 0..220)
		DrawRectangle(0, 88, 220, 672, {224, 231, 220, 255});
		DrawUiText("DANH MỤC CHỨC NĂNG", 22, 106, 12, {65, 84, 75, 255});

		if (currentUser) {
			auto drawNavBtn = [&](Rectangle rec, const char* label, bool isActive, bool isHover) {
				DrawRectangleRounded(rec, 0.2f, 4, isActive ? Color{35, 55, 53, 255} : (isHover ? Color{235, 240, 232, 255} : Color{224, 231, 220, 255}));
				DrawUiTextInRect(label, rec, 13, isActive ? RAYWHITE : Color{32, 55, 47, 255}, -1, 14.0f);
			};

			drawNavBtn({20, 134, 180, 44}, "1. BÁN HÀNG", activeTab == 0, hoverOrderTab);
			if (canAccessTab(currentUser, 1)) {
				drawNavBtn({20, 188, 180, 44}, "2. BÁO CÁO", activeTab == 1, hoverStatsTab);
			}
			drawNavBtn({20, 242, 180, 44}, "3. BẢN ĐỒ GIAO HÀNG", activeTab == 2, hoverMapTab);
			drawNavBtn({20, 296, 180, 44}, "4. KHO & THỰC ĐƠN", activeTab == 3, hoverInventoryTab);
			drawNavBtn({20, 350, 180, 44}, "5. LỊCH LÀM VIỆC", activeTab == 4, hoverScheduleTab);
			drawNavBtn({20, 404, 180, 44}, "6. ĐIỂM DANH", activeTab == 5, hoverAttendanceTab);

			Rectangle logoutRec = {20, 470, 180, 42};
			DrawRectangleRounded(logoutRec, 0.2f, 4, hoverLogout ? Color{255, 230, 215, 255} : Color{245, 232, 220, 255});
			DrawUiTextInRect("ĐĂNG XUẤT", logoutRec, 13, Color{195, 65, 50, 255}, 0, 10.0f);
		}

		DrawRectangleRounded({16, 678, 188, 64}, 0.15f, 4, Color{215, 224, 212, 255});
		DrawUiText("TRẠNG THÁI KẾT NỐI", 28, 690, 11, {65, 84, 75, 255});
		DrawUiTextFitted("• Máy chủ: Hoạt động tốt", 28, 712, 165, 12, {45, 108, 76, 255});

		// TAB 0: POS & ORDERING
		if (activeTab == 0) {
			DrawUiText("Tạo Đơn Hàng Mới (POS)", 245, 104, 24, {35, 55, 53, 255});

			// Search Bar
			Rectangle searchBox = {245, 148, 305, 36};
			DrawRectangleRounded(searchBox, 0.2f, 4, RAYWHITE);
			DrawRectangleRoundedLines(searchBox, 0.2f, 4, searchFocused ? Color{35, 105, 78, 255} : Color{202, 211, 201, 255});
			DrawUiTextInRect(search.empty() ? "Tìm theo tên hoặc mã món..." : search.c_str(),
				searchBox, 13, search.empty() ? Color{150, 158, 151, 255} : Color{45, 56, 50, 255}, -1, 12.0f);

			// Category filter pills (x = 562..916)
			const char* filterCategories[] = {"Tất cả", "Món chính", "Đồ uống", "Tráng miệng"};
			for (int c = 0; c < 4; ++c) {
				Rectangle catBtn = {562.0f + c * 90.0f, 148.0f, 84.0f, 36.0f};
				bool activeCat = categoryFilter == filterCategories[c];
				DrawRectangleRounded(catBtn, 0.25f, 4, activeCat ? Color{35, 105, 78, 255} : Color{235, 240, 234, 255});
				DrawUiTextInRect(filterCategories[c], catBtn, 12, activeCat ? RAYWHITE : Color{35, 55, 53, 255}, 0, 4.0f);
			}

			// Products Grid (2 columns, up to 8 items visible)
			int visibleIndex = 0;
			for (int i = 0; i < static_cast<int>(products.size()); ++i) {
				const Product& product = products[i];
				if (categoryFilter != "Tất cả" && product.category != categoryFilter) continue;
				std::string normalizedSearch = lowerText(search);
				if (!search.empty() && lowerText(product.name).find(normalizedSearch) == std::string::npos &&
					lowerText(product.code).find(normalizedSearch) == std::string::npos) continue;

				int column = visibleIndex % 2;
				int row = visibleIndex / 2;
				if (row >= 4) break;

				Rectangle card = {245.0f + column * 340.0f, 200.0f + row * 106.0f, 325, 94};
				DrawRectangleRounded(card, 0.08f, 4, RAYWHITE);
				DrawRectangleRoundedLines(card, 0.08f, 4, Color{225, 233, 226, 255});

				// Accent stripe
				DrawRectangle(static_cast<int>(card.x), static_cast<int>(card.y + 10), 6, 74, product.color);

				DrawUiTextFitted(product.name.c_str(), card.x + 16, card.y + 12, 295, 15, {42, 55, 48, 255});
				DrawUiTextFitted((product.code + "  •  " + product.category).c_str(), card.x + 16, card.y + 36, 185, 12, {126, 136, 127, 255});
				DrawUiTextFitted(formatCurrency(product.price).c_str(), card.x + 16, card.y + 60, 185, 14, {35, 105, 78, 255});

				// Add Button
				Rectangle addBtn = {card.x + 210, card.y + 54, 100, 30};
				DrawRectangleRounded(addBtn, 0.25f, 4, {35, 105, 78, 255});
				DrawUiTextInRect("+ Thêm", addBtn, 13, RAYWHITE, 0, 6.0f);

				visibleIndex++;
			}

			// Right Cart Panel (x = 935..1255, width = 320)
			Rectangle cartPanel = {935, 104, 320, 602};
			DrawRectangleRounded(cartPanel, 0.04f, 4, RAYWHITE);
			DrawRectangleRoundedLines(cartPanel, 0.04f, 4, Color{220, 228, 221, 255});

			DrawUiText("ĐƠN HÀNG HIỆN TẠI", 952, 120, 15, {35, 55, 53, 255});
			if (!cart.empty()) {
				Rectangle clearBtn = {1175, 116, 66, 26};
				DrawRectangleRounded(clearBtn, 0.2f, 4, Color{245, 230, 230, 255});
				DrawUiTextInRect("Xoá giỏ", clearBtn, 11, Color{195, 65, 50, 255}, 0, 4.0f);
			}

			DrawLine(952, 150, 1238, 150, {220, 228, 221, 255});

			if (cart.empty()) {
				DrawUiTextCenterFitted("Giỏ hàng đang trống", 1095, 245, 280, 14, {140, 150, 142, 255});
				DrawUiTextCenterFitted("Nhấp chọn món bên trái để thêm", 1095, 270, 280, 12, {160, 170, 162, 255});
			} else {
				int row = 0;
				for (const auto& item : cart) {
					if (row >= 8) break;
					float rowY = 162.0f + row * 34.0f;
					const Product* p = findProduct(products, item.productCode);
					std::string lineName = std::to_string(item.quantity) + "x " + (p ? p->name : item.productCode);

					DrawUiTextFitted(lineName.c_str(), 952, rowY + 3, 140, 13, {45, 56, 50, 255});

					int itemPrice = (p ? p->price : 0) * item.quantity;
					DrawUiTextRight(formatCurrency(itemPrice).c_str(), 1184, rowY + 3, 12, {35, 105, 78, 255});

					Rectangle minusBtn = {1190, rowY, 22, 22};
					DrawRectangleRounded(minusBtn, 0.2f, 2, Color{245, 235, 233, 255});
					DrawUiTextInRect("-", minusBtn, 13, {180, 70, 50, 255}, 0, 2.0f);

					Rectangle plusBtn = {1216, rowY, 22, 22};
					DrawRectangleRounded(plusBtn, 0.2f, 2, Color{234, 244, 238, 255});
					DrawUiTextInRect("+", plusBtn, 13, {35, 105, 78, 255}, 0, 2.0f);

					row++;
				}
			}

			// Calculation & Discounts Section
			int subtotal = cartTotal(cart, products);
			int discountAmount = computeDiscount(cart, products, discountPercent, comboPromotion);
			int finalTotal = std::max(0, subtotal - discountAmount);

			DrawLine(952, 444, 1238, 444, {220, 228, 221, 255});
			DrawUiText("Tạm tính:", 952, 452, 13, {80, 92, 84, 255});
			DrawUiTextRight(formatCurrency(subtotal).c_str(), 1238, 452, 13, {80, 92, 84, 255});

			// Discount percentage presets
			int presets[] = {0, 10, 20, 30, 50};
			for (int p = 0; p < 5; ++p) {
				Rectangle pBtn = {952.0f + p * 58.0f, 478.0f, 52.0f, 26.0f};
				bool active = discountPercent == presets[p];
				DrawRectangleRounded(pBtn, 0.2f, 4, active ? Color{180, 110, 40, 255} : Color{238, 242, 238, 255});
				DrawUiTextInRect((std::to_string(presets[p]) + "%").c_str(), pBtn, 12,
					active ? RAYWHITE : Color{45, 56, 50, 255}, 0, 2.0f);
			}

			// Custom % input box
			DrawUiText("Giảm giá tuỳ chỉnh (%):", 952, 514, 12, {80, 92, 84, 255});
			Rectangle discBox = {1158, 510, 80, 26};
			DrawRectangleRec(discBox, Color{248, 250, 248, 255});
			DrawRectangleLinesEx(discBox, 1.5f, discountInputFocused ? Color{35, 105, 78, 255} : Color{210, 218, 212, 255});
			DrawUiTextInRect((discountInputText + " %").c_str(), discBox, 12, {35, 55, 53, 255}, 0, 4.0f);

			// Combo promotion toggle button
			Rectangle comboBtn = {952, 544, 286, 28};
			DrawRectangleRounded(comboBtn, 0.2f, 4, comboPromotion ? Color{180, 110, 40, 255} : Color{245, 240, 230, 255});
			DrawUiTextInRect(comboPromotion ? "✓ Ưu đãi Combo (-15.000 đ)" : "Bật ưu đãi Combo (-15.000 đ)",
				comboBtn, 12, comboPromotion ? RAYWHITE : Color{140, 95, 45, 255}, 0, 6.0f);

			DrawUiText("Đã giảm:", 952, 580, 12, amber);
			DrawUiTextRight(("- " + formatCurrency(discountAmount)).c_str(), 1238, 580, 12, amber);

			// Final summary
			DrawUiText("Tổng thanh toán:", 952, 604, 14, {35, 55, 53, 255});
			DrawUiTextRight(formatCurrency(finalTotal).c_str(), 1238, 601, 17, {35, 105, 78, 255});

			// Pay Button
			Rectangle payBtn = {952, 634, 286, 42};
			DrawRectangleRounded(payBtn, 0.2f, 4, subtotal > 0 ? Color{35, 105, 78, 255} : Color{180, 192, 182, 255});
			DrawUiTextInRect("THANH TOÁN ĐƠN HÀNG", payBtn, 14, RAYWHITE, 0, 8.0f);
			if (orderPlaced) {
				DrawUiTextCenterFitted("Đã tạo đơn hàng thành công!", payBtn.x + payBtn.width / 2.0f, payBtn.y + 46, 280, 12, Color{35, 105, 78, 255});
			}

		} else if (activeTab == 1) {
			// TAB 1: REPORTS & SALES ANALYTICS
			const bool isBranchManager = currentUser->getRole() == Role::BranchManager;
			const ReportStats report = buildReportStats(orders, *currentUser, branchValues, selectedReportBranch);
			const int averageOrder = report.orderCount == 0 ? 0 : report.revenue / report.orderCount;

			DrawUiText("Báo cáo Doanh thu & Bán hàng", 245, 104, 24, {35, 55, 53, 255});

			if (isBranchManager) {
				DrawUiText("Bộ lọc:", 245, 144, 13, {90, 100, 94, 255});
				Rectangle allBtn = {310, 138, 115, 28};
				bool allActive = selectedReportBranch == -1;
				DrawRectangleRounded(allBtn, 0.2f, 4, allActive ? Color{35, 105, 78, 255} : Color{235, 240, 235, 255});
				DrawUiTextInRect("Tất cả CN", allBtn, 12, allActive ? RAYWHITE : darkGreen, 0, 4.0f);

				for (int b = 0; b < static_cast<int>(branchValues.size()); ++b) {
					Rectangle bBtn = {433.0f + b * 204.0f, 138.0f, 196.0f, 28.0f};
					bool bActive = selectedReportBranch == b;
					DrawRectangleRounded(bBtn, 0.2f, 4, bActive ? Color{35, 105, 78, 255} : Color{235, 240, 235, 255});
					DrawUiTextInRect(branchValues[b].getName().c_str(), bBtn, 12, bActive ? RAYWHITE : darkGreen, 0, 6.0f);
				}
			} else {
				std::string subTitle = "Phạm vi: " + currentUser->getBranchId() + " (Chi nhánh hiện tại của bạn)";
				DrawUiTextFitted(subTitle.c_str(), 245, 142, 980, 13, {104, 115, 108, 255});
			}

			// 4 Summary Metric Cards (x = 245..1245)
			const std::array<Rectangle, 4> cards{{
				{245, 176, 238, 84}, {499, 176, 238, 84}, {753, 176, 238, 84}, {1007, 176, 238, 84}
			}};
			const std::array<Color, 4> cardColors{{
				{35, 105, 78, 255}, {180, 110, 40, 255}, {60, 110, 130, 255}, {120, 85, 130, 255}
			}};
			const std::array<const char*, 4> cardLabels{{"TỔNG DOANH THU", "SỐ ĐƠN HÀNG", "TRUNG BÌNH / ĐƠN", "ĐƠN CHỜ XỬ LÝ"}};
			const std::array<std::string, 4> cardValues{{
				formatCurrency(report.revenue),
				std::to_string(report.orderCount) + " đơn",
				formatCurrency(averageOrder),
				std::to_string(report.pendingCount) + " đơn"
			}};

			for (int i = 0; i < 4; ++i) {
				DrawRectangleRounded(cards[i], 0.15f, 4, cardColors[i]);
				DrawUiTextFitted(cardLabels[i], cards[i].x + 16, cards[i].y + 14, 206, 12, {235, 242, 235, 255});
				DrawUiTextFitted(cardValues[i].c_str(), cards[i].x + 16, cards[i].y + 42, 206, 19, RAYWHITE);
			}

			if (isBranchManager) {
				DrawUiText("DOANH THU CHI TIẾT THEO TỪNG CHI NHÁNH", 245, 276, 14, darkGreen);

				int tableY = 300;
				DrawRectangle(245, tableY, 1000, 32, Color{232, 238, 233, 255});
				DrawUiText("CHI NHÁNH", 265, tableY + 8, 12, darkGreen);
				DrawUiText("MÃ CN", 525, tableY + 8, 12, darkGreen);
				DrawUiText("TỔNG ĐƠN", 635, tableY + 8, 12, darkGreen);
				DrawUiText("ĐƠN CHỜ", 755, tableY + 8, 12, darkGreen);
				DrawUiText("DOANH THU", 875, tableY + 8, 12, darkGreen);
				DrawUiText("TỶ TRỌNG", 1085, tableY + 8, 12, darkGreen);

				int rowY = tableY + 34;
				int totalChainRevenue = 0;
				for (const auto& bs : report.branchBreakdown) totalChainRevenue += bs.revenue;

				for (size_t b = 0; b < report.branchBreakdown.size(); ++b) {
					const auto& bs = report.branchBreakdown[b];
					DrawRectangle(245, rowY, 1000, 36, (b % 2 == 0) ? panel : panelAlt);
					DrawRectangleLines(245, rowY, 1000, 36, Color{230, 235, 230, 255});

					DrawUiTextFitted(bs.branchName.c_str(), 265, rowY + 10, 245, 13, darkGreen);
					DrawUiTextFitted(bs.branchId.c_str(), 525, rowY + 10, 95, 12, muted);
					DrawUiTextFitted((std::to_string(bs.totalOrders) + " đơn").c_str(), 635, rowY + 10, 105, 13, darkGreen);
					DrawUiTextFitted((std::to_string(bs.pendingOrders) + " đơn").c_str(), 755, rowY + 10, 105, 13, bs.pendingOrders > 0 ? amber : green);
					DrawUiTextFitted(formatCurrency(bs.revenue).c_str(), 875, rowY + 10, 190, 13, green);

					int percent = totalChainRevenue > 0 ? (bs.revenue * 100 / totalChainRevenue) : 0;
					DrawUiTextFitted((std::to_string(percent) + " %").c_str(), 1085, rowY + 10, 120, 13, darkGreen);

					rowY += 38;
				}

				DrawUiText("TOP MÓN ĂN BÁN CHẠY NHẤT", 245, 496, 14, darkGreen);
				int pY = 522;
				int pRow = 0;
				for (const auto& prod : products) {
					if (pRow >= 4) break;
					int qty = report.productQuantities.count(prod.code) ? report.productQuantities.at(prod.code) : 0;
					DrawRectangleRounded({245, static_cast<float>(pY), 1000, 36}, 0.1f, 4, panel);
					DrawRectangleRoundedLines({245, static_cast<float>(pY), 1000, 36}, 0.1f, 4, Color{230, 235, 230, 255});

					DrawUiTextFitted((std::to_string(pRow + 1) + ". " + prod.name + " (" + prod.code + ")").c_str(), 265, pY + 10, 460, 13, darkGreen);
					DrawUiTextFitted((std::to_string(qty) + " phần đã bán").c_str(), 755, pY + 10, 200, 13, muted);
					DrawUiTextFitted(formatCurrency(qty * prod.price).c_str(), 1015, pY + 10, 210, 13, green);

					pY += 42;
					pRow++;
				}
			} else {
				DrawUiText("SẢN PHẨM BÁN CHẠY TẠI CHI NHÁNH", 245, 286, 14, darkGreen);
				int pY = 316;
				int pRow = 0;
				for (const auto& prod : products) {
					if (pRow >= 7) break;
					int qty = report.productQuantities.count(prod.code) ? report.productQuantities.at(prod.code) : 0;
					DrawRectangleRounded({245, static_cast<float>(pY), 1000, 42}, 0.1f, 4, panel);
					DrawRectangleRoundedLines({245, static_cast<float>(pY), 1000, 42}, 0.1f, 4, Color{230, 235, 230, 255});

					DrawUiTextFitted((std::to_string(pRow + 1) + ". " + prod.name + " (" + prod.code + ")").c_str(), 265, pY + 12, 340, 14, darkGreen);
					DrawUiTextFitted(prod.category.c_str(), 625, pY + 12, 160, 13, muted);
					DrawUiTextFitted((std::to_string(qty) + " phần").c_str(), 810, pY + 12, 150, 13, darkGreen);
					DrawUiTextFitted(formatCurrency(qty * prod.price).c_str(), 990, pY + 12, 230, 14, green);

					pY += 48;
					pRow++;
				}
			}

		} else if (activeTab == 2) {
			// TAB 2: DELIVERY MAP & ORDER QUEUE
			DrawUiText("Bản Đồ Giao Hàng & Tìm Đường", 245, 104, 24, {35, 55, 53, 255});
			DrawUiTextFitted("Lưới toạ độ 30 x 20  •  Nhấp vào chi nhánh hoặc ô trống bất kỳ để tìm lộ trình ngắn nhất",
				245, 136, 1000, 13, {104, 115, 108, 255});

			const float mapW = mapGraph.width() * mapCellSize;  // 720
			const float mapH = mapGraph.height() * mapCellSize; // 480
			DrawRectangle(static_cast<int>(mapOriginX), static_cast<int>(mapOriginY), static_cast<int>(mapW), static_cast<int>(mapH), {235, 231, 218, 255});

			for (int y = 0; y < mapGraph.height(); ++y) {
				for (int x = 0; x < mapGraph.width(); ++x) {
					Rectangle cell = {mapOriginX + x * mapCellSize, mapOriginY + y * mapCellSize, mapCellSize, mapCellSize};
					DrawRectangleRec(cell, mapGraph.isBlocked(x, y) ? Color{104, 115, 108, 255} : Color{241, 238, 226, 255});
					DrawRectangleLines(static_cast<int>(cell.x), static_cast<int>(cell.y), static_cast<int>(mapCellSize), static_cast<int>(mapCellSize), {218, 214, 201, 255});
				}
			}

			const MapRoute& selectedRoute = useAStar ? aStarRoute : dijkstraRoute;
			int exploredCount = std::min(static_cast<int>(selectedRoute.explored.size()), static_cast<int>(pathAnimation));
			for (int i = 0; i < exploredCount; ++i) {
				int node = selectedRoute.explored[i];
				int nodeX = node % mapGraph.width();
				int nodeY = node / mapGraph.width();
				DrawRectangle(static_cast<int>(mapOriginX + nodeX * mapCellSize + 6), static_cast<int>(mapOriginY + nodeY * mapCellSize + 6), 12, 12, {184, 202, 191, 255});
			}

			int routeStart = static_cast<int>(selectedRoute.explored.size());
			int visibleRouteNodes = pathAnimation >= routeStart
				? std::min(static_cast<int>(selectedRoute.nodes.size()), static_cast<int>(pathAnimation) - routeStart)
				: 0;
			for (int i = 0; i < visibleRouteNodes; ++i) {
				int node = selectedRoute.nodes[i];
				int nodeX = node % mapGraph.width();
				int nodeY = node / mapGraph.width();
				DrawRectangle(static_cast<int>(mapOriginX + nodeX * mapCellSize + 4), static_cast<int>(mapOriginY + nodeY * mapCellSize + 4), 16, 16, useAStar ? Color{235, 142, 72, 255} : Color{35, 105, 78, 255});
			}

			for (int i = 0; i < static_cast<int>(branchValues.size()); ++i) {
				float branchX = mapOriginX + branchCells[i].x * mapCellSize + mapCellSize / 2.0f;
				float branchY = mapOriginY + branchCells[i].y * mapCellSize + mapCellSize / 2.0f;
				DrawCircle(static_cast<int>(branchX), static_cast<int>(branchY), 10, i == targetBranch ? Color{210, 92, 68, 255} : Color{35, 55, 53, 255});

				const std::string& bName = branchValues[i].getName();
				float tagW = MeasureUiText(bName.c_str(), 11) + 12.0f;
				float tagX = std::clamp(branchX - tagW / 2.0f, mapOriginX + 4.0f, mapOriginX + mapW - tagW - 4.0f);
				float tagY = std::clamp(branchY + 12.0f, mapOriginY + 4.0f, mapOriginY + mapH - 22.0f);
				Rectangle tagRec = {tagX, tagY, tagW, 18};
				DrawRectangleRounded(tagRec, 0.3f, 4, Color{255, 255, 255, 225});
				DrawUiTextInRect(bName.c_str(), tagRec, 11, {35, 55, 53, 255}, 0, 4.0f);
			}

			// Right Control Card Panel (x = 980..1255, width = 275, height = 480)
			Rectangle mapPanel = {980, 168, 275, 480};
			DrawRectangleRounded(mapPanel, 0.05f, 4, RAYWHITE);
			DrawRectangleRoundedLines(mapPanel, 0.05f, 4, Color{220, 228, 221, 255});

			float px = mapPanel.x + 15.0f;
			float pw = mapPanel.width - 30.0f; // 245px max width

			DrawUiTextFitted(useAStar ? "THUẬT TOÁN A* (TỐI ƯU)" : "THUẬT TOÁN DIJKSTRA", px, 186, pw, 12, {93, 112, 103, 255});

			std::string routeText = targetBranch >= 0
				? "Trung Tâm -> " + branchValues[targetBranch].getName()
				: "Trung Tâm -> Điểm tuỳ chọn";
			DrawUiTextFitted(routeText.c_str(), px, 210, pw, 14, {35, 105, 78, 255});

			char distanceText[100];
			std::snprintf(distanceText, sizeof(distanceText), "Quãng đường: %.0f ô  |  Duyệt: %d ô",
				selectedRoute.distance, static_cast<int>(selectedRoute.explored.size()));
			DrawUiTextFitted(distanceText, px, 238, pw, 13, {54, 66, 58, 255});

			DrawUiTextFitted(deliveryMode ? "Khuyến nghị: Giao tận nơi" : "Khuyến nghị: Dùng tại bàn", px, 266, pw, 13, {180, 110, 40, 255});

			std::string destStr = "Đích đến: " + (targetBranch >= 0 ? branchValues[targetBranch].getName() : "Toạ độ tuỳ chọn");
			DrawUiTextFitted(destStr.c_str(), px, 294, pw, 13, {54, 66, 58, 255});

			DrawLine(static_cast<int>(px), 322, static_cast<int>(px + pw), 322, {225, 232, 226, 255});

			char queueText[64];
			std::snprintf(queueText, sizeof(queueText), "Hàng đợi đơn hàng: %d đơn", orderQueue.size());
			DrawUiTextFitted(queueText, px, 334, pw, 14, {35, 105, 78, 255});

			DrawUiTextWrapped("Nhấn phím P hoặc bấm nút bên dưới để xử lý đơn hàng tiếp theo.",
				{px, 362, pw, 48}, 12, {104, 115, 108, 255});

			// Control Buttons inside mapPanel
			Rectangle serviceBtn = {px, 425, pw, 36};
			DrawRectangleRounded(serviceBtn, 0.2f, 4, {247, 239, 214, 255});
			DrawUiTextInRect(deliveryMode ? "Chế độ: Giao hàng" : "Chế độ: Dùng tại bàn", serviceBtn, 13, {130, 91, 48, 255}, 0, 8.0f);

			Rectangle algoBtn = {px, 470, pw, 36};
			DrawRectangleRounded(algoBtn, 0.2f, 4, {224, 231, 220, 255});
			DrawUiTextInRect(useAStar ? "Đổi sang: Thuật toán Dijkstra" : "Đổi sang: Thuật toán A*", algoBtn, 12, {54, 66, 58, 255}, 0, 8.0f);

			Rectangle processBtn = {px, 515, pw, 40};
			DrawRectangleRounded(processBtn, 0.2f, 4, orderQueue.empty() ? Color{180, 192, 182, 255} : Color{35, 105, 78, 255});
			DrawUiTextInRect("XỬ LÝ ĐƠN TIẾP THEO (Phím P)", processBtn, 13, RAYWHITE, 0, 8.0f);

		} else {
			// TABS 3, 4, 5: MANAGED BY ManagementUI
			managementUi.draw(mouse, activeTab, *currentUser);
		}

		// Bottom Status Bar (always on top of tab content at y = 722..760)
		DrawRectangle(220, 722, 1060, 38, Color{236, 232, 222, 255});
		DrawLine(220, 722, 1280, 722, Color{218, 214, 204, 255});
		DrawUiTextFitted(statusMessage.c_str(), 245, 733, 1010, 13, {35, 105, 78, 255});

		// LOGIN MODAL
		if (loginDialog) {
			DrawRectangle(0, 0, 1280, 760, {20, 30, 28, 165});

			DrawRectangleRounded(loginBoxRec, 0.05f, 8, {250, 247, 238, 255});
			DrawRectangleRoundedLines(loginBoxRec, 0.05f, 8, Color{35, 105, 78, 255});

			DrawUiTextFitted("HỆ THỐNG QUẢN LÝ BẾP NHỎ", 415, 182, 450, 21, {35, 55, 53, 255});
			DrawUiTextFitted("Đăng nhập tài khoản theo quyền hạn được cấp", 415, 214, 450, 13, {104, 115, 108, 255});

			DrawUiText("Tên đăng nhập:", 415, 252, 13, {54, 66, 58, 255});
			DrawRectangleRec(loginUserBoxRec, RAYWHITE);
			DrawRectangleLinesEx(loginUserBoxRec, 1.5f, loginField == 0 ? Color{35, 105, 78, 255} : Color{202, 211, 201, 255});
			DrawUiTextInRect(usernameInput.empty() ? "Nhập tài khoản (employee1, manager1, branchmanager)..." : usernameInput.c_str(),
				loginUserBoxRec, 13, usernameInput.empty() ? Color{150, 158, 151, 255} : Color{42, 55, 48, 255}, -1, 12.0f);

			DrawUiText("Mật khẩu:", 415, 332, 13, {54, 66, 58, 255});
			DrawRectangleRec(loginPassBoxRec, RAYWHITE);
			DrawRectangleLinesEx(loginPassBoxRec, 1.5f, loginField == 1 ? Color{35, 105, 78, 255} : Color{202, 211, 201, 255});
			std::string masked(passwordInput.size(), '*');
			DrawUiTextInRect(masked.empty() ? "Nhập mật khẩu..." : masked.c_str(),
				loginPassBoxRec, 14, masked.empty() ? Color{150, 158, 151, 255} : Color{42, 55, 48, 255}, -1, 12.0f);

			DrawUiTextFitted("Phím Tab để chuyển ô  •  Phím Enter để đăng nhập", 415, 412, 450, 12, {104, 115, 108, 255});

			DrawRectangleRounded(loginBtnRec, 0.2f, 4, Color{35, 105, 78, 255});
			DrawUiTextInRect("ĐĂNG NHẬP HỆ THỐNG", loginBtnRec, 14, RAYWHITE, 0, 12.0f);

			// Quick account reference box inside modal
			Rectangle hintBox = {415, 502, 450, 68};
			DrawRectangleRounded(hintBox, 0.12f, 4, Color{238, 242, 236, 255});
			DrawUiTextFitted("Tài khoản mẫu:  NV: employee1 / emp1pass", 427, 512, 426, 12, {65, 84, 75, 255});
			DrawUiTextFitted("QL: manager1 / mgr1pass  |  GĐ: branchmanager / branchpass", 427, 536, 426, 12, {65, 84, 75, 255});
		}

		// Tooltip
		if (!hoverHint.empty()) {
			float tooltipWidth = MeasureUiText(hoverHint.c_str(), 12) + 24.0f;
			float tooltipX = std::clamp(mouse.x + 14.0f, 8.0f, 1272.0f - tooltipWidth);
			float tooltipY = std::max(mouse.y - 34.0f, 92.0f);
			Rectangle tipRec = {tooltipX, tooltipY, tooltipWidth, 26};
			DrawRectangleRounded(tipRec, 0.25f, 4, {35, 55, 53, 245});
			DrawUiTextInRect(hoverHint.c_str(), tipRec, 12, RAYWHITE, 0, 8.0f);
		}

		EndMode2D();
		EndDrawing();
	}

	UnloadUiFont();
	CloseWindow();
	return 0;
}
