#pragma once

#include <string>
#include <vector>

class Schedule {
protected:
	std::string id;
	std::string date;
	std::string startTime;
	std::string endTime;
	int breakMinutes;
	std::string branchId;
	int requiredEmployees;
	std::vector<std::string> employeeIds;

public:
	Schedule(std::string scheduleId, std::string scheduleDate, std::string start,
		std::string end, int breakTime, std::string branch, int required = 1);
	virtual ~Schedule() = default;

	std::string getId() const;
	std::string getDate() const;
	std::string getStartTime() const;
	std::string getEndTime() const;
	int getBreakMinutes() const;
	std::string getBranchId() const;
	int getRequiredEmployees() const;
	const std::vector<std::string>& getEmployeeIds() const;
	int getAssignedEmployeeCount() const;
	bool hasCoverage() const;
	bool assignEmployee(const std::string& employeeId);
	virtual bool isOpenShift() const = 0;
};

class WorkSchedule : public Schedule {
public:
	using Schedule::Schedule;
};

class FullTimeSchedule : public WorkSchedule {
public:
	FullTimeSchedule(std::string scheduleId, std::string scheduleDate, std::string start,
		std::string end, int breakTime, std::string branch, int required = 1);
	bool isOpenShift() const override;
};

class SelfRegisteredSchedule : public WorkSchedule {
	bool openShift;

public:
	SelfRegisteredSchedule(std::string scheduleId, std::string scheduleDate, std::string start,
		std::string end, int breakTime, std::string branch, int required = 1,
		bool isOpen = true);
	bool isOpenShift() const override;
	bool canRegister() const;
};

class OperatingSchedule {
	std::string date;
	bool holiday;
	double holidayMultiplier;

public:
	OperatingSchedule(std::string operatingDate, bool isHoliday = false,
		double multiplier = 1.0);
	std::string getDate() const;
	bool isHoliday() const;
	double salaryModifier() const;
};