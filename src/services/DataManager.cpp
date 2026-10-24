#include "DataManager.h"

#include <cstdio>
#include <ctime>

bool DataManager::readEncryptedFile(const char* path, String& outPlain) {
	outPlain.clear();
	if (!path) return false;
	std::FILE* file = std::fopen(path, "rb");
	if (!file) return false;

	std::fseek(file, 0, SEEK_END);
	long fileSize = std::ftell(file);
	std::fseek(file, 0, SEEK_SET);

	if (fileSize <= 0) {
		std::fclose(file);
		return false;
	}

	outPlain.reserve(static_cast<std::size_t>(fileSize));
	unsigned char buffer[1024];
	std::size_t bytesRead = 0;
	while ((bytesRead = std::fread(buffer, 1, sizeof(buffer), file)) > 0) {
		for (std::size_t i = 0; i < bytesRead; ++i) {
			char decoded = static_cast<char>(buffer[i] ^ CIPHER_KEY);
			if (decoded != '\r') {
				outPlain.push_back(decoded);
			}
		}
	}
	std::fclose(file);
	return !outPlain.empty();
}

bool DataManager::writeEncryptedFile(const char* path, const String& plain) {
	if (!path) return false;
	String tempPath = String(path) + ".tmp";
	std::FILE* file = std::fopen(tempPath.c_str(), "wb");
	if (!file) return false;

	unsigned char buffer[1024];
	std::size_t total = plain.size();
	std::size_t offset = 0;
	while (offset < total) {
		std::size_t chunk = total - offset;
		if (chunk > sizeof(buffer)) chunk = sizeof(buffer);
		for (std::size_t i = 0; i < chunk; ++i) {
			buffer[i] = static_cast<unsigned char>(plain[offset + i]) ^ CIPHER_KEY;
		}
		if (std::fwrite(buffer, 1, chunk, file) != chunk) {
			std::fclose(file);
			std::remove(tempPath.c_str());
			return false;
		}
		offset += chunk;
	}
	std::fclose(file);
	std::remove(path);
	return std::rename(tempPath.c_str(), path) == 0;
}

Vector<String> DataManager::splitFields(const String& line, char delimiter) {
	Vector<String> fields;
	String current;
	for (std::size_t i = 0; i < line.size(); ++i) {
		char ch = line[i];
		if (ch == delimiter) {
			fields.push_back(current);
			current.clear();
		} else if (ch != '\r') {
			current.push_back(ch);
		}
	}
	if (!current.empty() || (!line.empty() && line.back() != delimiter)) {
		fields.push_back(current);
	}
	return fields;
}

Vector<String> DataManager::splitLines(const String& content) {
	Vector<String> lines;
	String current;
	for (std::size_t i = 0; i < content.size(); ++i) {
		char ch = content[i];
		if (ch == '\n') {
			if (!current.empty()) {
				lines.push_back(current);
				current.clear();
			}
		} else if (ch != '\r') {
			current.push_back(ch);
		}
	}
	if (!current.empty()) {
		lines.push_back(current);
	}
	return lines;
}

String DataManager::formatCurrency(int value) {
	String digits = toString(value);
	for (int pos = static_cast<int>(digits.size()) - 3; pos > 0; pos -= 3) {
		digits.insert(static_cast<std::size_t>(pos), ".");
	}
	return digits + " đ";
}

String DataManager::currentDateLabel() {
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
	return String(buffer);
}

String DataManager::currentTimestamp() {
	const std::time_t now = std::time(nullptr);
	std::tm localTime{};
#ifdef _WIN32
	localtime_s(&localTime, &now);
#else
	localtime_r(&now, &localTime);
#endif
	char buffer[32];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
	return String(buffer);
}

bool DataManager::matchBranch(const String& orderBranch, const Branch& branch) {
	if (orderBranch == branch.getName() || orderBranch == branch.getId()) return true;
	if (branch.getId() == "BR001" && (orderBranch == "Central Branch" || orderBranch == "Chi nhánh Trung Tâm")) return true;
	if (branch.getId() == "BR002" && (orderBranch == "West Branch" || orderBranch == "Chi nhánh Phía Tây")) return true;
	if (branch.getId() == "BR003" && (orderBranch == "East Branch" || orderBranch == "Chi nhánh Phía Đông")) return true;
	if (branch.getId() == "BR004" && (orderBranch == "South Branch" || orderBranch == "Chi nhánh Phía Nam")) return true;
	return false;
}

void DataManager::clearEmployeePool(Vector<Employee*>& employees) {
	for (int i = 0; i < employees.size(); ++i) {
		delete employees[i];
	}
	employees.clear();
}

void DataManager::rebuildEmployeePool(Vector<AccountRecord>& accounts,
	Vector<Employee*>& employees, Employee*& currentUser) {
	String currentUserId = currentUser ? currentUser->getId() : "";
	String currentUsername = currentUser ? currentUser->getUsername() : "";

	clearEmployeePool(employees);
	currentUser = nullptr;

	for (int i = 0; i < accounts.size(); ++i) {
		AccountRecord& acc = accounts[i];
		Employee* emp = nullptr;
		if (acc.role == "ChainManager") {
			emp = new ChainManager(acc.id, acc.name);
		} else if (acc.role == "Manager") {
			emp = new Manager(acc.id, acc.name, acc.branchId);
		} else {
			emp = new Employee(acc.id, acc.name, acc.branchId);
		}
		emp->setUsername(acc.username);
		acc.user = emp;
		employees.push_back(emp);

		if (!currentUserId.empty() && acc.id == currentUserId) {
			currentUser = emp;
		} else if (!currentUsername.empty() && acc.username == currentUsername) {
			currentUser = emp;
		}
	}
}

bool DataManager::loadAccounts(Vector<AccountRecord>& accounts) {
	accounts.clear();
	String plain;
	if (!readEncryptedFile("data/accounts.dat", plain)) return false;

	Vector<String> lines = splitLines(plain);
	for (int i = 0; i < lines.size(); ++i) {
		Vector<String> fields = splitFields(lines[i], '|');
		if (fields.size() != 7) continue;
		accounts.push_back(AccountRecord(fields[0], fields[1], fields[2], fields[3], fields[4], fields[5], fields[6], nullptr));
	}
	return !accounts.empty();
}

bool DataManager::saveAccounts(const Vector<AccountRecord>& accounts) {
	String content;
	for (int i = 0; i < accounts.size(); ++i) {
		const AccountRecord& acc = accounts[i];
		content += acc.id + "|" + acc.username + "|" + acc.name + "|"
			+ acc.role + "|" + acc.branchId + "|" + acc.salt + "|" + acc.passwordHash + "\n";
	}
	return writeEncryptedFile("data/accounts.dat", content);
}

bool DataManager::loadBranches(Vector<Branch>& branches, RestaurantChain& chain) {
	branches.clear();
	chain.clear();
	String plain;
	if (!readEncryptedFile("data/branches.dat", plain)) {
		branches.push_back(Branch("BR001", "Chi nhánh Bách Khoa", 12.0f, 3.0f));
		branches.push_back(Branch("BR002", "Chi nhánh Chợ Hoà Khánh", 12.0f, 13.0f));
		branches.push_back(Branch("BR003", "Chi nhánh Ngô Sĩ Liên", 21.0f, 5.0f));
		branches.push_back(Branch("BR004", "Chi nhánh Nam Cao", 19.0f, 18.0f));
		for (int i = 0; i < branches.size(); ++i) {
			chain.addBranch(branches[i].getId(), branches[i].getName(), branches[i].getX(), branches[i].getY());
		}
		saveBranches(branches);
		return true;
	}
	Vector<String> lines = splitLines(plain);
	for (int i = 0; i < lines.size(); ++i) {
		Vector<String> fields = splitFields(lines[i], '|');
		if (fields.size() < 4) continue;
		float gx = fields[2].toFloat(12.0f);
		float gy = fields[3].toFloat(3.0f);
		Branch b(fields[0], fields[1], gx, gy);
		if (fields.size() >= 6) {
			b.setOpeningHours(fields[4], fields[5]);
		}
		branches.push_back(b);
		chain.addBranch(b.getId(), b.getName(), gx, gy);
	}
	return !branches.empty();
}

bool DataManager::saveBranches(const Vector<Branch>& branches) {
	String content;
	for (int i = 0; i < branches.size(); ++i) {
		const Branch& b = branches[i];
		content += b.getId() + "|" + b.getName() + "|"
			+ toString(static_cast<int>(b.getX())) + "|" + toString(static_cast<int>(b.getY())) + "|"
			+ b.getOpeningHours().getOpenTime() + "|" + b.getOpeningHours().getCloseTime() + "\n";
	}
	return writeEncryptedFile("data/branches.dat", content);
}

bool DataManager::loadMenu(Vector<Product>& products, MenuLookup& menuLookup) {
	products.clear();
	menuLookup.clear();
	String plain;
	if (!readEncryptedFile("data/menu.dat", plain)) {
		products = {
			Product("M101", "Bánh Mì Đặc Biệt", "Món chính", 35000, Color{210, 92, 68, 255}),
			Product("M102", "Cơm Gà Nướng Mật Ong", "Món chính", 55000, Color{210, 92, 68, 255}),
			Product("M103", "Mì Xào Bò Rau Cải", "Món chính", 48000, Color{210, 92, 68, 255}),
			Product("M104", "Phở Bò Tái Nạm", "Món chính", 60000, Color{210, 92, 68, 255}),
			Product("M105", "Trà Đào Cam Sả", "Đồ uống", 28000, Color{82, 150, 128, 255}),
			Product("M106", "Cà Phê Sữa Đá", "Đồ uống", 25000, Color{82, 150, 128, 255}),
			Product("M107", "Nước Cam Ép Tươi", "Đồ uống", 30000, Color{82, 150, 128, 255}),
			Product("M108", "Bánh Flan Caramel", "Tráng miệng", 22000, Color{192, 122, 155, 255})
		};
		for (int i = 0; i < products.size(); ++i) {
			menuLookup.addItem(MenuItem(products[i].code, products[i].name, static_cast<float>(products[i].price)));
		}
		saveMenu(products);
		return true;
	}
	Vector<String> lines = splitLines(plain);
	for (int i = 0; i < lines.size(); ++i) {
		Vector<String> fields = splitFields(lines[i], '|');
		if (fields.size() < 4) continue;
		int price = 0;
		if (!fields[3].tryParseInt(price)) continue;
		Color col = {235, 142, 72, 255};
		if (fields[2] == "Món chính") col = {210, 92, 68, 255};
		else if (fields[2] == "Đồ uống") col = {82, 150, 128, 255};
		else if (fields[2] == "Tráng miệng") col = {192, 122, 155, 255};
		products.push_back(Product(fields[0], fields[1], fields[2], price, col));
		menuLookup.addItem(MenuItem(fields[0], fields[1], static_cast<float>(price)));
	}
	return !products.empty();
}

bool DataManager::saveMenu(const Vector<Product>& products) {
	String content;
	for (int i = 0; i < products.size(); ++i) {
		const Product& p = products[i];
		content += p.code + "|" + p.name + "|" + p.category + "|" + toString(p.price) + "\n";
	}
	return writeEncryptedFile("data/menu.dat", content);
}

bool DataManager::loadIngredients(Vector<IngredientType>& ingredients) {
	ingredients.clear();
	String plain;
	if (!readEncryptedFile("data/ingredients.dat", plain)) {
		ingredients = {
			IngredientType("FLOUR", "Bột mì", "kg"),
			IngredientType("NOODLES", "Mì sợi", "kg"),
			IngredientType("BEEF", "Thịt bò tươi", "kg"),
			IngredientType("CHICKEN", "Thịt gà", "kg"),
			IngredientType("COFFEE", "Cà phê hạt", "kg"),
			IngredientType("MILK", "Sữa đặc", "hộp")
		};
		saveIngredients(ingredients);
		return true;
	}
	Vector<String> lines = splitLines(plain);
	for (int i = 0; i < lines.size(); ++i) {
		Vector<String> fields = splitFields(lines[i], '|');
		if (fields.size() < 3) continue;
		ingredients.push_back(IngredientType(fields[0], fields[1], fields[2]));
	}
	if (ingredients.empty()) {
		ingredients = {
			IngredientType("FLOUR", "Bột mì", "kg"),
			IngredientType("NOODLES", "Mì sợi", "kg")
		};
	}
	return true;
}

bool DataManager::saveIngredients(const Vector<IngredientType>& ingredients) {
	String content;
	for (int i = 0; i < ingredients.size(); ++i) {
		const IngredientType& ing = ingredients[i];
		content += ing.id + "|" + ing.name + "|" + ing.unit + "\n";
	}
	return writeEncryptedFile("data/ingredients.dat", content);
}

bool DataManager::loadInventory(Vector<Branch>& branches) {
	String plain;
	if (!readEncryptedFile("data/inventory.dat", plain)) return false;
	Vector<String> lines = splitLines(plain);
	for (int i = 0; i < lines.size(); ++i) {
		Vector<String> fields = splitFields(lines[i], '|');
		if (fields.size() != 3) continue;
		int quantity = 0;
		if (!fields[2].tryParseInt(quantity) || quantity < 0) continue;
		for (int b = 0; b < branches.size(); ++b) {
			if (branches[b].getId() == fields[0]) {
				branches[b].addStock(fields[1], quantity);
			}
		}
	}
	return true;
}

bool DataManager::saveInventory(const Vector<Branch>& branches, const Vector<IngredientType>& ingredients) {
	String content;
	for (int b = 0; b < branches.size(); ++b) {
		const Branch& branch = branches[b];
		for (int i = 0; i < ingredients.size(); ++i) {
			const IngredientType& ing = ingredients[i];
			content += branch.getId() + "|" + ing.id + "|" + toString(branch.getStock(ing.id)) + "\n";
		}
	}
	return writeEncryptedFile("data/inventory.dat", content);
}

bool DataManager::loadOrders(Vector<Order>& orders) {
	orders.clear();
	String plain;
	if (!readEncryptedFile("data/orders.dat", plain)) return false;
	Vector<String> lines = splitLines(plain);
	for (int i = 0; i < lines.size(); ++i) {
		Vector<String> fields = splitFields(lines[i], '|');
		if (fields.size() != 7) continue;
		Order order{};
		if (!fields[0].tryParseInt(order.id) || !fields[1].tryParseInt(order.total)) continue;
		order.branch = fields[2];
		order.orderId = fields[3];
		order.customerId = fields[4];
		order.status = fields[5];
		Vector<String> rawItems = splitFields(fields[6], ',');
		for (int j = 0; j < rawItems.size(); ++j) {
			Vector<String> itemFields = splitFields(rawItems[j], ':');
			if (itemFields.size() != 2) continue;
			int quantity = 0;
			if (!itemFields[1].tryParseInt(quantity) || quantity <= 0) continue;
			order.items.push_back(OrderItem(itemFields[0], quantity));
		}
		orders.push_back(order);
	}
	return true;
}

bool DataManager::saveOrders(const Vector<Order>& orders) {
	String content;
	for (int i = 0; i < orders.size(); ++i) {
		const Order& order = orders[i];
		content += toString(order.id) + "|" + toString(order.total) + "|" + order.branch + "|"
			+ order.orderId + "|" + order.customerId + "|" + order.status + "|";
		for (int j = 0; j < order.items.size(); ++j) {
			if (j > 0) content += ",";
			content += order.items[j].productCode + ":" + toString(order.items[j].quantity);
		}
		content += "\n";
	}
	return writeEncryptedFile("data/orders.dat", content);
}

bool DataManager::loadSchedules(ScheduleService& scheduleService) {
	scheduleService.clear();
	String plain;
	if (!readEncryptedFile("data/schedules.dat", plain)) return false;
	Vector<String> lines = splitLines(plain);
	for (int i = 0; i < lines.size(); ++i) {
		Vector<String> fields = splitFields(lines[i], '|');
		if (fields.size() != 7 && fields.size() != 8) continue;
		int breakMinutes = 0;
		int requiredEmployees = 0;
		if (!fields[4].tryParseInt(breakMinutes) || !fields[6].tryParseInt(requiredEmployees)) continue;
		SelfRegisteredSchedule* schedule = scheduleService.openOpenShift(
			fields[0], fields[1], fields[2], fields[3], breakMinutes, fields[5], requiredEmployees);
		if (fields.size() == 8 && schedule) {
			Vector<String> empIds = splitFields(fields[7], ',');
			for (int j = 0; j < empIds.size(); ++j) {
				if (!empIds[j].empty()) schedule->assignEmployee(empIds[j]);
			}
		}
	}
	return true;
}

bool DataManager::saveSchedules(const ScheduleService& scheduleService) {
	String content;
	const Vector<Schedule*>& schedules = scheduleService.getSchedules();
	for (int i = 0; i < schedules.size(); ++i) {
		const Schedule* schedule = schedules[i];
		content += schedule->getId() + "|" + schedule->getDate() + "|"
			+ schedule->getStartTime() + "|" + schedule->getEndTime() + "|"
			+ toString(schedule->getBreakMinutes()) + "|" + schedule->getBranchId() + "|"
			+ toString(schedule->getRequiredEmployees()) + "|";
		const Vector<String>& empIds = schedule->getEmployeeIds();
		for (int j = 0; j < empIds.size(); ++j) {
			if (j > 0) content += ",";
			content += empIds[j];
		}
		content += "\n";
	}
	return writeEncryptedFile("data/schedules.dat", content);
}

bool DataManager::loadAttendance(Vector<Timekeeping>& records) {
	String plain;
	if (!readEncryptedFile("data/attendance.dat", plain)) return false;
	Vector<String> lines = splitLines(plain);
	for (int i = 0; i < lines.size(); ++i) {
		Vector<String> fields = splitFields(lines[i], '|');
		if (fields.size() != 5) continue;
		for (int r = 0; r < records.size(); ++r) {
			if (records[r].getEmployeeId() == fields[1]) {
				records[r].addRecord(AttendanceRecord(fields[0], fields[1], fields[2], fields[3], fields[4] == "1"));
			}
		}
	}
	return true;
}

bool DataManager::saveAttendance(const Vector<Timekeeping>& records) {
	String content;
	for (int r = 0; r < records.size(); ++r) {
		const Vector<AttendanceRecord>& items = records[r].getRecords();
		for (int i = 0; i < items.size(); ++i) {
			const AttendanceRecord& item = items[i];
			content += item.scheduleId + "|" + item.employeeId + "|" + item.checkIn + "|"
				+ item.checkOut + "|" + (item.isPresent ? "1" : "0") + "\n";
		}
	}
	return writeEncryptedFile("data/attendance.dat", content);
}
