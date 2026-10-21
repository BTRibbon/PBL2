#pragma once

#include "../models/Branch.h"
#include "../models/RestaurantChain.h"
#include "../models/Timekeeping.h"
#include "../models/User.h"
#include "../datastructures/MenuLookup.h"
#include "../services/ScheduleService.h"
#include "../services/ShiftManager.h"
#include "raylib.h"

#include <string>
#include <vector>

struct Product {
	std::string code;
	std::string name;
	std::string category;
	int price;
	Color color;
};

class ManagementUI {
	RestaurantChain& chain;
	std::vector<Branch>& branches;
	ScheduleService& scheduleService;
	ShiftManager& shiftManager;
	std::vector<Timekeeping>& timekeepingRecords;
	std::vector<Employee*>& employees;
	std::vector<Product>& products;
	MenuLookup& menuLookup;

	std::string message;
	std::string dateInput = "2026-10-10";
	std::string startInput = "08:00";
	std::string endInput = "16:00";
	int focusedField = -1;

	// Sub-tab in Tab 3: 0 = Thực đơn, 1 = Kho & Chi nhánh
	int managementSubTab = 0;

	// Menu dialog (Add / Edit product)
	bool isMenuDialogOpen = false;
	int menuDialogMode = 0; // 0: Add, 1: Edit
	int editingProductIndex = -1;
	std::string menuCodeInput;
	std::string menuNameInput;
	std::string menuCategoryInput = "Món chính";
	std::string menuPriceInput;
	int menuDialogField = 0; // 0: Code, 1: Name, 2: Category, 3: Price

	void drawButton(Rectangle bounds, const char* label, bool enabled = true, Color bgColor = {35, 105, 78, 255}) const;
	void drawManagementHeader(const char* title, const char* subtitle) const;
	void drawInventoryTab(Vector2 mouse, User& currentUser);
	void drawMenuTab(Vector2 mouse, User& currentUser);
	void drawMenuDialog();
	void drawScheduleTab(Vector2 mouse, User& currentUser);
	void drawAttendanceTab(Vector2 mouse, User& currentUser);
	Timekeeping* getTimekeeping(User& currentUser) const;

public:
	ManagementUI(RestaurantChain& restaurantChain, std::vector<Branch>& branchValues,
		ScheduleService& service, ShiftManager& manager,
		std::vector<Timekeeping>& records, std::vector<Employee*>& employeeUsers,
		std::vector<Product>& productList, MenuLookup& lookup);

	bool handleClick(Vector2 mouse, int activeTab, User& currentUser);
	void updateInput(int key, int charPressed);
	void draw(Vector2 mouse, int activeTab, User& currentUser);
	const std::string& getMessage() const;
	void setMessage(const std::string& msg);
	bool isDialogOpen() const { return isMenuDialogOpen; }
	int getSubTab() const { return managementSubTab; }
	void setSubTab(int sub) { managementSubTab = sub; }
};
