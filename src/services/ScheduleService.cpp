#include "ScheduleService.h"

#include "../models/User.h"

ScheduleService::ScheduleService(ShiftManager* manager) : shiftManager(manager) {}

void ScheduleService::addSchedule(const std::shared_ptr<Schedule>& schedule) {
	if (schedule) schedules.push_back(schedule);
}

bool ScheduleService::deleteSchedule(const std::string& scheduleId) {
	for (auto it = schedules.begin(); it != schedules.end(); ++it) {
		if ((*it)->getId() == scheduleId) {
			schedules.erase(it);
			return true;
		}
	}
	return false;
}

std::shared_ptr<SelfRegisteredSchedule> ScheduleService::openOpenShift(
	std::string scheduleId, std::string date, std::string start, std::string end,
	int breakTime, std::string branch, int required) {
	auto schedule = std::make_shared<SelfRegisteredSchedule>(scheduleId, date, start, end,
		breakTime, branch, required, true);
	addSchedule(schedule);
	return schedule;
}

std::shared_ptr<Schedule> ScheduleService::findSchedule(const std::string& scheduleId) const {
	for (const auto& schedule : schedules) if (schedule->getId() == scheduleId) return schedule;
	return nullptr;
}

bool ScheduleService::registerForEmptyShift(Employee& employee, const std::string& scheduleId) {
	auto schedule = findSchedule(scheduleId);
	if (!schedule || !schedule->isOpenShift()) return false;
	bool branchMatches = (employee.branchId == "ALL") ||
		(schedule->getBranchId() == "ALL") ||
		(schedule->getBranchId() == employee.branchId);
	return branchMatches && schedule->assignEmployee(employee.getId());
}

bool ScheduleService::validateMinimumShifts(const Employee& employee, const std::string& fromDate,
	const std::string& toDate) const {
	int registered = 0;
	for (const auto& schedule : schedules) {
		if (schedule->getDate() < fromDate || schedule->getDate() > toDate) continue;
		for (const std::string& employeeId : schedule->getEmployeeIds())
			if (employeeId == employee.getId()) ++registered;
	}
	return registered >= employee.getMinimumShifts();
}

std::vector<std::shared_ptr<Schedule>> ScheduleService::checkShiftCoverage(const std::string& branchId) const {
	std::vector<std::shared_ptr<Schedule>> uncovered;
	const bool checkAll = branchId.empty() || branchId == "ALL";
	for (const auto& schedule : schedules) {
		if (!checkAll && schedule->getBranchId() != branchId && schedule->getBranchId() != "ALL") continue;
		if (!schedule->hasCoverage()) uncovered.push_back(schedule);
	}
	return uncovered;
}

void ScheduleService::notifyShiftManager(const std::string& branchId) {
	if (!shiftManager) return;
	shiftManager->clearAlerts();
	for (const auto& schedule : checkShiftCoverage(branchId)) {
		shiftManager->notifyMissingCoverage(schedule->getId(),
			schedule->getRequiredEmployees() - schedule->getAssignedEmployeeCount(),
			schedule->getBranchId());
	}
}

const std::vector<std::shared_ptr<Schedule>>& ScheduleService::getSchedules() const { return schedules; }
