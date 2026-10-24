#include "ScheduleService.h"
#include "../models/Employee.h"

ScheduleService::ScheduleService(ShiftManager* manager) : shiftManager(manager) {}

ScheduleService::~ScheduleService() {
	clear();
}

void ScheduleService::clear() {
	for (int i = 0; i < schedules.size(); ++i) {
		delete schedules[i];
	}
	schedules.clear();
}

void ScheduleService::addSchedule(Schedule* schedule) {
	if (schedule) schedules.push_back(schedule);
}

bool ScheduleService::deleteSchedule(const String& scheduleId) {
	for (int i = 0; i < schedules.size(); ++i) {
		if (schedules[i]->getId() == scheduleId) {
			delete schedules[i];
			schedules.eraseAt(i);
			return true;
		}
	}
	return false;
}

SelfRegisteredSchedule* ScheduleService::openOpenShift(
	const String& scheduleId, const String& date, const String& start, const String& end,
	int breakTime, const String& branch, int required) {
	SelfRegisteredSchedule* schedule = new SelfRegisteredSchedule(scheduleId, date, start, end,
		breakTime, branch, required, true);
	addSchedule(schedule);
	return schedule;
}

Schedule* ScheduleService::findSchedule(const String& scheduleId) const {
	for (int i = 0; i < schedules.size(); ++i) {
		if (schedules[i]->getId() == scheduleId) return schedules[i];
	}
	return nullptr;
}

bool ScheduleService::registerForEmptyShift(Employee& employee, const String& scheduleId) {
	Schedule* schedule = findSchedule(scheduleId);
	if (!schedule || !schedule->isOpenShift()) return false;
	bool branchMatches = (employee.getBranchId() == "ALL") ||
		(schedule->getBranchId() == "ALL") ||
		(schedule->getBranchId() == employee.getBranchId());
	return branchMatches && schedule->assignEmployee(employee.getId());
}

bool ScheduleService::validateMinimumShifts(const Employee& employee, const String& fromDate,
	const String& toDate) const {
	int registered = 0;
	for (int i = 0; i < schedules.size(); ++i) {
		const Schedule* schedule = schedules[i];
		if (schedule->getDate() < fromDate || schedule->getDate() > toDate) continue;
		const Vector<String>& empIds = schedule->getEmployeeIds();
		for (int j = 0; j < empIds.size(); ++j) {
			if (empIds[j] == employee.getId()) ++registered;
		}
	}
	return registered >= employee.getMinimumShifts();
}

Vector<Schedule*> ScheduleService::checkShiftCoverage(const String& branchId) const {
	Vector<Schedule*> uncovered;
	const bool checkAll = branchId.empty() || branchId == "ALL";
	for (int i = 0; i < schedules.size(); ++i) {
		Schedule* schedule = schedules[i];
		if (!checkAll && schedule->getBranchId() != branchId && schedule->getBranchId() != "ALL") continue;
		if (!schedule->hasCoverage()) uncovered.push_back(schedule);
	}
	return uncovered;
}

void ScheduleService::notifyShiftManager(const String& branchId) {
	if (!shiftManager) return;
	shiftManager->clearAlerts();
	Vector<Schedule*> uncovered = checkShiftCoverage(branchId);
	for (int i = 0; i < uncovered.size(); ++i) {
		Schedule* schedule = uncovered[i];
		shiftManager->notifyMissingCoverage(schedule->getId(),
			schedule->getRequiredEmployees() - schedule->getAssignedEmployeeCount(),
			schedule->getBranchId());
	}
}

const Vector<Schedule*>& ScheduleService::getSchedules() const { return schedules; }
