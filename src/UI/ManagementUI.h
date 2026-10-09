#pragma once

#include "../models/Branch.h"
#include "../models/RestaurantChain.h"
#include "../models/Timekeeping.h"
#include "../models/User.h"
#include "../services/ScheduleService.h"
#include "../services/ShiftManager.h"
#include "raylib.h"

#include <string>
#include <vector>

class ManagementUI {
	RestaurantChain& chain;
	std::vector<Branch>& branches;
	ScheduleService& scheduleService;
	ShiftManager& shiftManager;
	std::vector<Timekeeping>& timekeepingRecords;
	std::vector<Employee*>& employees;
	std::string message;
	std::string dateInput = "2026-10-10";
	std::string startInput = "08:00";
	std::string endInput = "16:00";
	int focusedField = -1;

	void drawButton(Rectangle bounds, const char* label, bool enabled = true) const;
	void drawManagementHeader(const char* title, const char* subtitle) const;
	void drawInventoryTab(Vector2 mouse, User& currentUser) const;
	void drawScheduleTab(Vector2 mouse, User& currentUser);
	void drawAttendanceTab(Vector2 mouse, User& currentUser);
	Timekeeping* getTimekeeping(User& currentUser) const;

public:
	ManagementUI(RestaurantChain& restaurantChain, std::vector<Branch>& branchValues,
		ScheduleService& service, ShiftManager& manager,
		std::vector<Timekeeping>& records, std::vector<Employee*>& employeeUsers);

	bool handleClick(Vector2 mouse, int activeTab, User& currentUser);
	void updateInput(int key);
	void draw(Vector2 mouse, int activeTab, User& currentUser);
	const std::string& getMessage() const;
};
