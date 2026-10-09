#pragma once

#include "ShiftManager.h"
#include "../models/Schedule.h"
#include <memory>
#include <string>
#include <vector>

class Employee;

class ScheduleService {
	std::vector<std::shared_ptr<Schedule>> schedules;
	ShiftManager* shiftManager;

	std::shared_ptr<Schedule> findSchedule(const std::string& scheduleId) const;

public:
	explicit ScheduleService(ShiftManager* manager = nullptr);
	void addSchedule(const std::shared_ptr<Schedule>& schedule);
	bool deleteSchedule(const std::string& scheduleId);
	std::shared_ptr<SelfRegisteredSchedule> openOpenShift(
		std::string scheduleId, std::string date, std::string start, std::string end,
		int breakTime, std::string branch, int required = 1);
	bool registerForEmptyShift(Employee& employee, const std::string& scheduleId);
	bool validateMinimumShifts(const Employee& employee, const std::string& fromDate,
		const std::string& toDate) const;
	std::vector<std::shared_ptr<Schedule>> checkShiftCoverage() const;
	void notifyShiftManager();
	const std::vector<std::shared_ptr<Schedule>>& getSchedules() const;
};