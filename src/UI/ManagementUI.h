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

struct IngredientType {
	std::string id;
	std::string name;
	std::string unit;
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
	std::vector<IngredientType>& ingredientTypes;

	std::string message;
	std::string dateInput = "2026-10-21";
	std::string startInput = "08:00";
	std::string endInput = "16:00";
	int focusedField = -1;
	int scheduleBranchIndex = 0;

	// Sub-tab in Tab 3: 0 = Thực đơn, 1 = Kho & Nguyên liệu
	int managementSubTab = 0;
	int selectedInventoryBranch = 0;

	// Scroll offsets
	int menuScrollOffset = 0;
	int ingredientScrollOffset = 0;
	int invBranchScrollOffset = 0;
	int scheduleScrollOffset = 0;
	int attendanceScrollOffset = 0;

	// Menu dialog (Add / Edit product - BranchManager only)
	bool isMenuDialogOpen = false;
	int menuDialogMode = 0; // 0: Add, 1: Edit
	int editingProductIndex = -1;
	std::string menuCodeInput;
	std::string menuNameInput;
	std::string menuCategoryInput = "Món chính";
	std::string menuPriceInput;
	int menuDialogField = 0;

	// Ingredient Type dialog (Add ingredient type - BranchManager only)
	bool isIngredientDialogOpen = false;
	std::string ingIdInput;
	std::string ingNameInput;
	std::string ingUnitInput = "kg";
	std::string ingInitialStockInput = "50";
	int ingDialogField = 0;

	// Animation state
	double dialogOpenTime = 0.0;
	bool wasDialogOpen = false;
	double subTabChangeTime = -10.0;
	int lastSubTab = 0;

	void drawButton(Rectangle bounds, const char* label, Vector2 mouse, bool enabled = true, Color bgColor = {35, 105, 78, 255}) const;
	void drawPill(Rectangle bounds, const char* label, Vector2 mouse, bool selected, float fontSize = 13.0f) const;
	float beginDialogTransition(Rectangle dialog) const;
	void endDialogTransition() const;
	void drawManagementHeader(const char* title, const char* subtitle) const;
	void drawInventoryTab(Vector2 mouse, User& currentUser);
	void drawMenuTab(Vector2 mouse, User& currentUser);
	void drawMenuDialog(Vector2 mouse);
	void drawIngredientDialog(Vector2 mouse);
	void drawScheduleTab(Vector2 mouse, User& currentUser);
	void drawAttendanceTab(Vector2 mouse, User& currentUser);
	Timekeeping* getTimekeeping(User& currentUser) const;
	bool submitMenuDialog();
	bool submitIngredientDialog();

public:
	ManagementUI(RestaurantChain& restaurantChain, std::vector<Branch>& branchValues,
		ScheduleService& service, ShiftManager& manager,
		std::vector<Timekeeping>& records, std::vector<Employee*>& employeeUsers,
		std::vector<Product>& productList, MenuLookup& lookup,
		std::vector<IngredientType>& ingredients);

	bool handleClick(Vector2 mouse, int activeTab, User& currentUser);
	void updateScroll(Vector2 mouse, float wheelMove, int activeTab, User& currentUser);
	bool updateInput(int key, int charPressed);
	void draw(Vector2 mouse, int activeTab, User& currentUser);
	const std::string& getMessage() const;
	void setMessage(const std::string& msg);
	bool isDialogOpen() const { return isMenuDialogOpen || isIngredientDialogOpen; }
	int getSubTab() const { return managementSubTab; }
	void setSubTab(int sub) { managementSubTab = sub; }
};
