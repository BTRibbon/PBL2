#include "Schedule.h"

#include <algorithm>

Schedule::Schedule(std::string scheduleId, std::string scheduleDate, std::string start,
	std::string end, int breakTime, std::string branch, int required)
	: id(scheduleId), date(scheduleDate), startTime(start), endTime(end),
	  breakMinutes(std::max(0, breakTime)), branchId(branch),
	  requiredEmployees(std::max(1, required)) {}

std::string Schedule::getId() const { return id; }
std::string Schedule::getDate() const { return date; }
std::string Schedule::getStartTime() const { return startTime; }
std::string Schedule::getEndTime() const { return endTime; }
int Schedule::getBreakMinutes() const { return breakMinutes; }
std::string Schedule::getBranchId() const { return branchId; }
int Schedule::getRequiredEmployees() const { return requiredEmployees; }
const std::vector<std::string>& Schedule::getEmployeeIds() const { return employeeIds; }
int Schedule::getAssignedEmployeeCount() const { return static_cast<int>(employeeIds.size()); }
bool Schedule::hasCoverage() const { return getAssignedEmployeeCount() >= requiredEmployees; }

bool Schedule::assignEmployee(const std::string& employeeId) {
	if (employeeId.empty() || hasCoverage()) return false;
	if (std::find(employeeIds.begin(), employeeIds.end(), employeeId) != employeeIds.end()) return false;
	employeeIds.push_back(employeeId);
	return true;
}

FullTimeSchedule::FullTimeSchedule(std::string scheduleId, std::string scheduleDate,
	std::string start, std::string end, int breakTime, std::string branch, int required)
	: WorkSchedule(scheduleId, scheduleDate, start, end, breakTime, branch, required) {}

bool FullTimeSchedule::isOpenShift() const { return false; }

SelfRegisteredSchedule::SelfRegisteredSchedule(std::string scheduleId, std::string scheduleDate,
	std::string start, std::string end, int breakTime, std::string branch, int required, bool isOpen)
	: WorkSchedule(scheduleId, scheduleDate, start, end, breakTime, branch, required), openShift(isOpen) {}

bool SelfRegisteredSchedule::isOpenShift() const { return openShift; }
bool SelfRegisteredSchedule::canRegister() const { return openShift && !hasCoverage(); }

OperatingSchedule::OperatingSchedule(std::string operatingDate, bool isHoliday, double multiplier)
	: date(operatingDate), holiday(isHoliday), holidayMultiplier(multiplier > 0.0 ? multiplier : 1.0) {}

std::string OperatingSchedule::getDate() const { return date; }
bool OperatingSchedule::isHoliday() const { return holiday; }
double OperatingSchedule::salaryModifier() const { return holiday ? holidayMultiplier : 1.0; }