#pragma once

#include "raylib.h"
#include "../datastructures/String.h"
#include "../datastructures/Vector.h"
#include "../datastructures/MenuLookup.h"
#include "../models/Branch.h"
#include "../models/RestaurantChain.h"
#include "../models/Timekeeping.h"
#include "../models/Employee.h"
#include "../services/DataManager.h"
#include "../services/ScheduleService.h"
#include "../services/ShiftManager.h"

class ManagementUI {
private:
	RestaurantChain& chain;
	Vector<Branch>& branches;
	ScheduleService& scheduleService;
	ShiftManager& shiftManager;
	Vector<Timekeeping>& timekeepingRecords;
	Vector<Employee*>& employees;
	Vector<Product>& products;
	MenuLookup& menuLookup;
	Vector<IngredientType>& ingredientTypes;

	String message;
	String dateInput = "2026-10-21";
	String startInput = "08:00";
	String endInput = "16:00";
	int focusedField = -1;
	int scheduleBranchIndex = 0;

	int managementSubTab = 0;
	int selectedInventoryBranch = 0;

	int menuScrollOffset = 0;
	int ingredientScrollOffset = 0;
	int invBranchScrollOffset = 0;
	int scheduleScrollOffset = 0;
	int attendanceScrollOffset = 0;

	// Hộp thoại Thực đơn (Thêm / Sửa món ăn - chỉ Giám đốc chuỗi ChainManager)
	bool isMenuDialogOpen = false;
	int menuDialogMode = 0; // 0: Thêm, 1: Sửa
	int editingProductIndex = -1;
	String menuCodeInput;
	String menuNameInput;
	String menuCategoryInput = "Món chính";
	String menuPriceInput;
	int menuDialogField = 0;

	// Hộp thoại Nguyên liệu (Thêm loại nguyên liệu - chỉ Giám đốc chuỗi ChainManager)
	bool isIngredientDialogOpen = false;
	String ingIdInput;
	String ingNameInput;
	String ingUnitInput = "kg";
	String ingInitialStockInput = "50";
	int ingDialogField = 0;

	double dialogOpenTime = 0.0;
	bool wasDialogOpen = false;
	double subTabChangeTime = -10.0;
	int lastSubTab = 0;

	void drawButton(Rectangle bounds, const char* label, Vector2 mouse, bool enabled = true, Color bgColor = {35, 105, 78, 255}) const;
	void drawPill(Rectangle bounds, const char* label, Vector2 mouse, bool selected, float fontSize = 13.0f) const;
	float beginDialogTransition(Rectangle dialog) const;
	void endDialogTransition() const;
	void drawManagementHeader(const char* title, const char* subtitle) const;
	void drawInventoryTab(Vector2 mouse, Employee& currentUser);
	void drawMenuTab(Vector2 mouse, Employee& currentUser);
	void drawMenuDialog(Vector2 mouse);
	void drawIngredientDialog(Vector2 mouse);
	void drawScheduleTab(Vector2 mouse, Employee& currentUser);
	void drawAttendanceTab(Vector2 mouse, Employee& currentUser);
	Timekeeping* getTimekeeping(Employee& currentUser) const;
	bool submitMenuDialog();
	bool submitIngredientDialog();

public:
	ManagementUI(RestaurantChain& restaurantChain, Vector<Branch>& branchValues,
		ScheduleService& service, ShiftManager& manager,
		Vector<Timekeeping>& records, Vector<Employee*>& employeeUsers,
		Vector<Product>& productList, MenuLookup& lookup,
		Vector<IngredientType>& ingredients);

	bool updateInput(int key, int charPressed);
	void updateScroll(Vector2 mouse, float wheelMove, int activeTab, Employee& currentUser);
	bool handleClick(Vector2 mouse, int activeTab, Employee& currentUser);
	void draw(Vector2 mouse, int activeTab, Employee& currentUser);
	const String& getMessage() const;
	void setMessage(const String& msg);
	bool isDialogOpen() const { return isMenuDialogOpen || isIngredientDialogOpen; }
};
