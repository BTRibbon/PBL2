#include "Employee.h"
#include "../services/ScheduleService.h"

Employee::Employee(const String& empId, const String& empName, const String& branch)
	: id(empId), name(empName), username(empName), branchId(branch), role(Role::Employee), minimumShifts(0) {}

String Employee::getId() const { return id; }

String Employee::getName() const { return name; }

void Employee::setName(const String& value) { name = value; }

String Employee::getUsername() const { return username; }

void Employee::setUsername(const String& value) { username = value; }

String Employee::getBranchId() const { return branchId; }

void Employee::setBranchId(const String& value) { branchId = value; }

int Employee::getMinimumShifts() const { return minimumShifts; }

void Employee::setMinimumShifts(int minimum) { minimumShifts = minimum < 0 ? 0 : minimum; }

bool Employee::registerForEmptyShift(ScheduleService& scheduleService, const String& scheduleId) {
	return scheduleService.registerForEmptyShift(*this, scheduleId);
}

Role Employee::getRole() const { return Role::Employee; }

const char* Employee::getRoleTitle() const { return "Nhân viên"; }

bool Employee::canManageBranches() const { return false; }

bool Employee::canManageAllBranches() const { return false; }

bool Employee::canViewReports() const { return false; }

bool Employee::canAccessTab(int tabId) const {
	// Nhân viên được truy cập: Bán hàng (0), Bản đồ (2), Kho & Thực đơn (3), Lịch làm (4), Điểm danh (5)
	return tabId == 0 || (tabId >= 2 && tabId <= 5);
}

Manager::Manager(const String& mgrId, const String& mgrName, const String& branch)
	: Employee(mgrId, mgrName, branch) {
	role = Role::Manager;
}

Role Manager::getRole() const { return Role::Manager; }

const char* Manager::getRoleTitle() const { return "Quản lý chi nhánh"; }

bool Manager::canManageBranches() const { return true; }

bool Manager::canViewReports() const { return true; }

bool Manager::canAccessTab(int tabId) const {
	// Quản lý chi nhánh được truy cập toàn bộ 7 tab (0..6)
	return tabId >= 0 && tabId <= 6;
}

ChainManager::ChainManager(const String& cmId, const String& cmName)
	: Manager(cmId, cmName, "ALL") {
	role = Role::ChainManager;
}

Role ChainManager::getRole() const { return Role::ChainManager; }

const char* ChainManager::getRoleTitle() const { return "Giám đốc chuỗi"; }

bool ChainManager::canManageAllBranches() const { return true; }

bool ChainManager::canAccessTab(int tabId) const {
	return tabId >= 0 && tabId <= 6;
}

