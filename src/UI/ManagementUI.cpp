#include "ManagementUI.h"

#include "../models/MenuItem.h"

#include <cstdio>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <cctype>

namespace {
const Color darkGreen{35, 55, 53, 255};
const Color green{35, 105, 78, 255};
const Color muted{104, 115, 108, 255};
const Color panel{255, 255, 255, 255};
}

ManagementUI::ManagementUI(RestaurantChain& restaurantChain, std::vector<Branch>& branchValues,
	ScheduleService& service, ShiftManager& manager, std::vector<Timekeeping>& records,
	std::vector<Employee*>& employeeUsers)
	: chain(restaurantChain), branches(branchValues), scheduleService(service),
	  shiftManager(manager), timekeepingRecords(records), employees(employeeUsers) {}

static std::string currentTimestamp() {
	auto now = std::chrono::system_clock::now();
	std::time_t current = std::chrono::system_clock::to_time_t(now);
	std::tm localTime{};
#ifdef _WIN32
	localtime_s(&localTime, &current);
#else
	localtime_r(&current, &localTime);
#endif
	char buffer[32];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
	return buffer;
}

static bool validDate(const std::string& value) {
	return value.size() == 10 && value[4] == '-' && value[7] == '-' &&
		std::isdigit(static_cast<unsigned char>(value[0])) &&
		std::isdigit(static_cast<unsigned char>(value[1])) &&
		std::isdigit(static_cast<unsigned char>(value[2])) &&
		std::isdigit(static_cast<unsigned char>(value[3])) &&
		std::isdigit(static_cast<unsigned char>(value[5])) &&
		std::isdigit(static_cast<unsigned char>(value[6])) &&
		std::isdigit(static_cast<unsigned char>(value[8])) &&
		std::isdigit(static_cast<unsigned char>(value[9]));
}

static bool validTime(const std::string& value) {
	if (value.size() != 5 || value[2] != ':') return false;
	if (!std::isdigit(static_cast<unsigned char>(value[0])) ||
		!std::isdigit(static_cast<unsigned char>(value[1])) ||
		!std::isdigit(static_cast<unsigned char>(value[3])) ||
		!std::isdigit(static_cast<unsigned char>(value[4]))) return false;
	int hours = std::stoi(value.substr(0, 2));
	int minutes = std::stoi(value.substr(3, 2));
	return hours < 24 && minutes < 60;
}

Timekeeping* ManagementUI::getTimekeeping(User& currentUser) const {
	Employee* currentEmployee = dynamic_cast<Employee*>(&currentUser);
	if (!currentEmployee) return nullptr;
	for (size_t i = 0; i < employees.size() && i < timekeepingRecords.size(); ++i)
		if (employees[i] == currentEmployee) return &timekeepingRecords[i];
	return nullptr;
}

void ManagementUI::updateInput(int key) {
	if (focusedField < 0 || key <= 0) return;
	std::string* target = focusedField == 0 ? &dateInput : focusedField == 1 ? &startInput : &endInput;
	if (key == KEY_BACKSPACE) {
		if (!target->empty()) target->pop_back();
		return;
	}
	if (key >= 32 && key <= 126 && target->size() < 16) target->push_back(static_cast<char>(key));
}

void ManagementUI::drawButton(Rectangle bounds, const char* label, bool enabled) const {
	DrawRectangleRec(bounds, enabled ? green : Color{172, 184, 173, 255});
	DrawText(label, static_cast<int>(bounds.x) + 12, static_cast<int>(bounds.y) + 11, 14, RAYWHITE);
}

void ManagementUI::drawManagementHeader(const char* title, const char* subtitle) const {
	DrawText(title, 250, 118, 28, darkGreen);
	DrawText(subtitle, 250, 155, 14, muted);
}

void ManagementUI::drawInventoryTab(Vector2 mouse, User& currentUser) const {
	drawManagementHeader("Inventory", "Add or remove ingredients for the permitted branch");

	for (int i = 0; i < static_cast<int>(branches.size()); ++i) {
		Employee* currentEmployee = dynamic_cast<Employee*>(&currentUser);
		if (currentEmployee && !currentUser.canManageAllBranches() &&
			branches[i].getId() != currentEmployee->branchId) continue;
		int y = 190 + i * 105;
		DrawRectangle(250, y, 930, 88, panel);
		DrawText(branches[i].getName().c_str(), 270, y + 12, 17, darkGreen);
		char details[128];
		std::snprintf(details, sizeof(details), "Flour: %d | Noodles: %d | Revenue: %d | %s-%s",
			branches[i].getStock("FLOUR"), branches[i].getStock("NOODLES"), branches[i].getRevenue(),
			branches[i].getOpeningHours().getOpenTime().c_str(),
			branches[i].getOpeningHours().getCloseTime().c_str());
		DrawText(details, 270, y + 42, 13, muted);

		drawButton({670.0f, static_cast<float>(y + 16), 145, 34}, "+10 flour");
		drawButton({825.0f, static_cast<float>(y + 16), 145, 34}, "-5 flour");
		drawButton({980.0f, static_cast<float>(y + 16), 170, 34}, "+10 noodles");
		drawButton({670.0f, static_cast<float>(y + 55), 145, 28}, "-5 noodles");
		drawButton({825.0f, static_cast<float>(y + 55), 145, 28}, "Add M101");
		drawButton({980.0f, static_cast<float>(y + 55), 170, 28}, "Set hours");
		(void)mouse;
	}
}

void ManagementUI::drawScheduleTab(Vector2 mouse, User& currentUser) {
	drawManagementHeader("Schedules", "Create open shifts, register and inspect coverage");
	bool isEmployee = currentUser.getRole() == Role::Employee ||
		currentUser.getRole() == Role::Manager || currentUser.getRole() == Role::BranchManager;
	std::string dateLabel = "Date: " + dateInput;
	std::string startLabel = "Start: " + startInput;
	std::string endLabel = "End: " + endInput;
	drawButton({250, 180, 145, 38}, "Create shift", currentUser.canManageBranches());
	DrawRectangleLines(410, 180, 145, 38, focusedField == 0 ? green : Color{202, 211, 201, 255});
	DrawRectangleLines(570, 180, 145, 38, focusedField == 1 ? green : Color{202, 211, 201, 255});
	DrawRectangleLines(730, 180, 145, 38, focusedField == 2 ? green : Color{202, 211, 201, 255});
	DrawText(dateLabel.c_str(), 420, 191, 12, darkGreen);
	DrawText(startLabel.c_str(), 580, 191, 12, darkGreen);
	DrawText(endLabel.c_str(), 740, 191, 12, darkGreen);
	drawButton({890, 180, 220, 38}, "Notify coverage", currentUser.canManageBranches());
	DrawText("Click a field, type the value, then press Create shift. Date: YYYY-MM-DD, time: HH:MM.",
		250, 225, 12, muted);

	DrawText("ALL SHIFTS", 250, 250, 14, darkGreen);
	int y = 275;
	for (const auto& schedule : scheduleService.getSchedules()) {
		DrawRectangle(250, y, 860, 74, panel);
		char details[180];
		std::snprintf(details, sizeof(details), "%s | Date: %s | %s-%s | branch %s",
			schedule->getId().c_str(), schedule->getDate().c_str(),
			schedule->getStartTime().c_str(), schedule->getEndTime().c_str(),
			schedule->getBranchId().c_str());
		DrawText(details, 270, y + 12, 14, darkGreen);
		std::snprintf(details, sizeof(details), "%d/%d %s",
			schedule->getAssignedEmployeeCount(), schedule->getRequiredEmployees(),
			schedule->hasCoverage() ? "covered" : "missing");
		DrawText(details, 270, y + 42, 13, muted);
		if (isEmployee && schedule->isOpenShift())
			drawButton({850, static_cast<float>(y + 12), 100, 28}, "Join");
		if (currentUser.canManageBranches())
			drawButton({970, static_cast<float>(y + 12), 100, 28}, "Delete");
		y += 88;
	}
	int alertY = 430;
	for (const std::string& alert : shiftManager.getAlerts()) {
		DrawText(alert.c_str(), 930, alertY, 13, Color{177, 105, 54, 255});
		alertY += 26;
	}
	Employee* currentEmployee = dynamic_cast<Employee*>(&currentUser);
	if (currentEmployee && scheduleService.validateMinimumShifts(*currentEmployee, "2026-01-01", "2026-12-31"))
		DrawText("Minimum shift requirement: satisfied", 930, 255, 14, green);
	else
		DrawText("Minimum shift requirement: not satisfied", 930, 255, 14, Color{177, 105, 54, 255});
	(void)mouse;
}

void ManagementUI::drawAttendanceTab(Vector2 mouse, User& currentUser) {
	drawManagementHeader("Attendance", "Record check-in and check-out using system time");
	bool isEmployee = currentUser.getRole() == Role::Employee ||
		currentUser.getRole() == Role::Manager || currentUser.getRole() == Role::BranchManager;
	drawButton({250, 180, 160, 38}, "CHECK IN", isEmployee);
	drawButton({430, 180, 160, 38}, "CHECK OUT", isEmployee);
	Timekeeping* records = getTimekeeping(currentUser);
	DrawText(("Employee: " + (records ? records->getEmployeeId() : "unknown")).c_str(), 250, 250, 16, darkGreen);

	int y = 290;
	if (records) for (const AttendanceRecord& record : records->getRecords()) {
		char details[180];
		std::snprintf(details, sizeof(details), "%s | in: %s | out: %s | %s",
			record.scheduleId.c_str(), record.checkIn.c_str(), record.checkOut.c_str(),
			record.isPresent ? "present" : "absent");
		DrawText(details, 270, y, 14, muted);
		y += 28;
	}
	if (!message.empty()) DrawText(message.c_str(), 250, 650, 14, green);
	(void)mouse;
}

bool ManagementUI::handleClick(Vector2 mouse, int activeTab, User& currentUser) {
	if (activeTab == 3) {
		for (int i = 0; i < static_cast<int>(branches.size()); ++i) {
			Employee* currentEmployee = dynamic_cast<Employee*>(&currentUser);
			if (currentEmployee && !currentUser.canManageAllBranches() && branches[i].getId() != currentEmployee->branchId) continue;
			int y = 190 + i * 105;
			if (CheckCollisionPointRec(mouse, {670.0f, static_cast<float>(y + 16), 145, 34})) {
				branches[i].addStock("FLOUR", 10);
				message = "Added 10 kg flour to " + branches[i].getName();
				return true;
			}
			if (CheckCollisionPointRec(mouse, {825.0f, static_cast<float>(y + 16), 145, 34})) {
				branches[i].removeStock("FLOUR", 5);
				message = "Removed 5 kg flour from " + branches[i].getName();
				return true;
			}
			if (CheckCollisionPointRec(mouse, {980.0f, static_cast<float>(y + 16), 170, 34})) {
				branches[i].addStock("NOODLES", 10);
				message = "Added 10 kg noodles to " + branches[i].getName();
				return true;
			}
			if (CheckCollisionPointRec(mouse, {670.0f, static_cast<float>(y + 55), 145, 28})) {
				branches[i].removeStock("NOODLES", 5);
				message = "Removed 5 kg noodles from " + branches[i].getName();
				return true;
			}
			if (CheckCollisionPointRec(mouse, {825.0f, static_cast<float>(y + 55), 145, 28})) {
				branches[i].addMenuItem({"M101", "Special Banh Mi", 35000.0f});
				message = "Added M101 to " + branches[i].getName();
				return true;
			}
			if (CheckCollisionPointRec(mouse, {980.0f, static_cast<float>(y + 55), 170, 28})) {
				branches[i].setOpeningHours("09:00", "23:00");
				message = "Opening hours updated for " + branches[i].getName();
				return true;
			}
		}
	}

	if (activeTab == 4) {
		if (CheckCollisionPointRec(mouse, {250, 180, 145, 38}) && currentUser.canManageBranches()) {
			if (!validDate(dateInput) || !validTime(startInput) || !validTime(endInput) || startInput >= endInput) {
				message = "Invalid date/time. Use YYYY-MM-DD and HH:MM.";
				return true;
			}
			Employee* currentEmployee = dynamic_cast<Employee*>(&currentUser);
			scheduleService.openOpenShift("OPEN" + std::to_string(scheduleService.getSchedules().size() + 1),
				dateInput, startInput, endInput, 30, currentEmployee ? currentEmployee->branchId : "BR001", 2);
			message = "Created a new open shift";
			return true;
		}
		if (CheckCollisionPointRec(mouse, {410, 180, 145, 38}) && currentUser.canManageBranches()) focusedField = 0;
		if (CheckCollisionPointRec(mouse, {570, 180, 145, 38}) && currentUser.canManageBranches()) focusedField = 1;
		if (CheckCollisionPointRec(mouse, {730, 180, 145, 38}) && currentUser.canManageBranches()) focusedField = 2;
		if (CheckCollisionPointRec(mouse, {890, 180, 220, 38}) && currentUser.canManageBranches()) {
			scheduleService.notifyShiftManager();
			message = "Coverage alerts refreshed";
			return true;
		}
		int scheduleIndex = 0;
		std::string scheduleToDelete;
		for (const auto& schedule : scheduleService.getSchedules()) {
			int cardY = 275 + scheduleIndex * 88;
			if (schedule->isOpenShift() && CheckCollisionPointRec(mouse, {850, static_cast<float>(cardY + 12), 100, 28})) {
				Employee* currentEmployee = dynamic_cast<Employee*>(&currentUser);
				message = currentEmployee && currentEmployee->registerForEmptyShift(scheduleService, schedule->getId())
					? "Shift registered successfully" : "Cannot register this shift";
				return true;
			}
			if (currentUser.canManageBranches() &&
				CheckCollisionPointRec(mouse, {970, static_cast<float>(cardY + 12), 100, 28})) {
				scheduleToDelete = schedule->getId();
				break;
			}
			++scheduleIndex;
		}
		if (!scheduleToDelete.empty()) {
			message = scheduleService.deleteSchedule(scheduleToDelete)
				? "Shift deleted: " + scheduleToDelete
				: "Could not delete shift";
			return true;
		}
	}

	if (activeTab == 5) {
		if (CheckCollisionPointRec(mouse, {250, 180, 160, 38}) &&
			(currentUser.getRole() == Role::Employee || currentUser.getRole() == Role::Manager ||
			 currentUser.getRole() == Role::BranchManager)) {
			Timekeeping* records = getTimekeeping(currentUser);
			if (records) message = records->checkIn("OPEN1", currentTimestamp()) ? "Checked in" : "Already checked in";
			return true;
		}
		if (CheckCollisionPointRec(mouse, {430, 180, 160, 38}) &&
			(currentUser.getRole() == Role::Employee || currentUser.getRole() == Role::Manager ||
			 currentUser.getRole() == Role::BranchManager)) {
			Timekeeping* records = getTimekeeping(currentUser);
			if (records) message = records->checkOut("OPEN1", currentTimestamp()) ? "Checked out" : "No open check-in";
			return true;
		}
	}
	return false;
}

void ManagementUI::draw(Vector2 mouse, int activeTab, User& currentUser) {
	if (activeTab == 3) drawInventoryTab(mouse, currentUser);
	else if (activeTab == 4) drawScheduleTab(mouse, currentUser);
	else if (activeTab == 5) drawAttendanceTab(mouse, currentUser);
}

const std::string& ManagementUI::getMessage() const {
	return message;
}
