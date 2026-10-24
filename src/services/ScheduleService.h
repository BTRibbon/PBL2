#pragma once

#include "ShiftManager.h"
#include "../models/Schedule.h"
#include "../datastructures/String.h"
#include "../datastructures/Vector.h"

class Employee;

// Dịch vụ quản lý Lịch làm việc & Phân ca (sử dụng mảng con trỏ đa hình Schedule*)
class ScheduleService {
private:
	Vector<Schedule*> schedules;
	ShiftManager* shiftManager;

	Schedule* findSchedule(const String& scheduleId) const;

public:
	explicit ScheduleService(ShiftManager* manager = nullptr);
	ScheduleService(const ScheduleService&) = delete;
	ScheduleService& operator=(const ScheduleService&) = delete;
	~ScheduleService();

	void clear();
	void addSchedule(Schedule* schedule);
	bool deleteSchedule(const String& scheduleId);
	SelfRegisteredSchedule* openOpenShift(
		const String& scheduleId, const String& date, const String& start, const String& end,
		int breakTime, const String& branch, int required = 1);
	bool registerForEmptyShift(Employee& employee, const String& scheduleId);
	bool validateMinimumShifts(const Employee& employee, const String& fromDate,
		const String& toDate) const;
	Vector<Schedule*> checkShiftCoverage(const String& branchId = "") const;
	void notifyShiftManager(const String& branchId = "");
	const Vector<Schedule*>& getSchedules() const;
};
