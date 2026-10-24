#pragma once

#include "raylib.h"
#include "../datastructures/String.h"
#include "../datastructures/Vector.h"
#include "../datastructures/MenuLookup.h"
#include "../models/AccountRecord.h"
#include "../models/Branch.h"
#include "../models/Employee.h"
#include "../models/Order.h"
#include "../models/RestaurantChain.h"
#include "../models/Timekeeping.h"
#include "ScheduleService.h"

struct Product {
	String code;
	String name;
	String category;
	int price = 0;
	Color color{210, 92, 68, 255};

	Product() = default;
	Product(const String& pCode, const String& pName, const String& pCat, int pPrice, Color pCol)
		: code(pCode), name(pName), category(pCat), price(pPrice), color(pCol) {}
};

struct IngredientType {
	String id;
	String name;
	String unit;

	IngredientType() = default;
	IngredientType(const String& iId, const String& iName, const String& iUnit)
		: id(iId), name(iName), unit(iUnit) {}
};

// Dịch vụ Quản lý Dữ liệu & Lưu trữ file nhị phân mã hóa (.dat)
class DataManager {
public:
	static constexpr unsigned char CIPHER_KEY = 0x5A;

	// Đọc / ghi file nhị phân có mã hóa XOR 0x5A
	static bool readEncryptedFile(const char* path, String& outPlain);
	static bool writeEncryptedFile(const char* path, const String& plain);

	// Tiện ích xử lý chuỗi, thời gian và tiền tệ
	static Vector<String> splitFields(const String& line, char delimiter = '|');
	static Vector<String> splitLines(const String& content);
	static String formatCurrency(int value);
	static String currentDateLabel();
	static String currentTimestamp();
	static bool matchBranch(const String& orderBranch, const Branch& branch);

	// Quản lý danh sách đối tượng Employee đa hình
	static void rebuildEmployeePool(Vector<AccountRecord>& accounts,
		Vector<Employee*>& employees, Employee*& currentUser);
	static void clearEmployeePool(Vector<Employee*>& employees);

	// Nạp & Lưu các bảng dữ liệu (.dat)
	static bool loadAccounts(Vector<AccountRecord>& accounts);
	static bool saveAccounts(const Vector<AccountRecord>& accounts);

	static bool loadBranches(Vector<Branch>& branches, RestaurantChain& chain);
	static bool saveBranches(const Vector<Branch>& branches);

	static bool loadMenu(Vector<Product>& products, MenuLookup& menuLookup);
	static bool saveMenu(const Vector<Product>& products);

	static bool loadIngredients(Vector<IngredientType>& ingredients);
	static bool saveIngredients(const Vector<IngredientType>& ingredients);

	static bool loadInventory(Vector<Branch>& branches);
	static bool saveInventory(const Vector<Branch>& branches, const Vector<IngredientType>& ingredients);

	static bool loadOrders(Vector<Order>& orders);
	static bool saveOrders(const Vector<Order>& orders);

	static bool loadSchedules(ScheduleService& scheduleService);
	static bool saveSchedules(const ScheduleService& scheduleService);

	static bool loadAttendance(Vector<Timekeeping>& records);
	static bool saveAttendance(const Vector<Timekeeping>& records);
};

inline bool SaveAccountsToFile(const Vector<AccountRecord>& accounts) {
	return DataManager::saveAccounts(accounts);
}

