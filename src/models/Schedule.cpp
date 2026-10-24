#include "Schedule.h"

Schedule::Schedule(const String& scheduleId, const String& scheduleDate, const String& start,
	const String& end, int breakTime, const String& branch, int required)
	: id(scheduleId), date(scheduleDate), startTime(start), endTime(end),
	  breakMinutes(maxVal(0, breakTime)), branchId(branch),
	  requiredEmployees(maxVal(1, required)) {}

String Schedule::getId() const { return id; }
String Schedule::getDate() const { return date; }
String Schedule::getStartTime() const { return startTime; }
String Schedule::getEndTime() const { return endTime; }
int Schedule::getBreakMinutes() const { return breakMinutes; }
String Schedule::getBranchId() const { return branchId; }
int Schedule::getRequiredEmployees() const { return requiredEmployees; }
const Vector<String>& Schedule::getEmployeeIds() const { return employeeIds; }
int Schedule::getAssignedEmployeeCount() const { return employeeIds.size(); }
bool Schedule::hasCoverage() const { return getAssignedEmployeeCount() >= requiredEmployees; }

bool Schedule::assignEmployee(const String& employeeId) {
	if (employeeId.empty() || hasCoverage()) return false;
	if (employeeIds.contains(employeeId)) return false;
	employeeIds.push_back(employeeId);
	return true;
}

FullTimeSchedule::FullTimeSchedule(const String& scheduleId, const String& scheduleDate,
	const String& start, const String& end, int breakTime, const String& branch, int required)
	: WorkSchedule(scheduleId, scheduleDate, start, end, breakTime, branch, required) {}

bool FullTimeSchedule::isOpenShift() const { return false; }

SelfRegisteredSchedule::SelfRegisteredSchedule(const String& scheduleId, const String& scheduleDate,
	const String& start, const String& end, int breakTime, const String& branch, int required, bool isOpen)
	: WorkSchedule(scheduleId, scheduleDate, start, end, breakTime, branch, required), openShift(isOpen) {}

bool SelfRegisteredSchedule::isOpenShift() const { return openShift; }
bool SelfRegisteredSchedule::canRegister() const { return openShift && !hasCoverage(); }

OperatingSchedule::OperatingSchedule(const String& operatingDate, bool isHoliday, double multiplier)
	: date(operatingDate), holiday(isHoliday), holidayMultiplier(multiplier > 0.0 ? multiplier : 1.0) {}

String OperatingSchedule::getDate() const { return date; }
bool OperatingSchedule::isHoliday() const { return holiday; }
double OperatingSchedule::salaryModifier() const { return holiday ? holidayMultiplier : 1.0; }
