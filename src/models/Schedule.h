#pragma once

#include "../datastructures/String.h"
#include "../datastructures/Vector.h"

// Lớp trừu tượng (Abstract Class) biểu diễn Ca làm việc
class Schedule {
protected:
	String id;
	String date;
	String startTime;
	String endTime;
	int breakMinutes;
	String branchId;
	int requiredEmployees;
	Vector<String> employeeIds;

public:
	Schedule(const String& scheduleId, const String& scheduleDate, const String& start,
		const String& end, int breakTime, const String& branch, int required = 1);
	virtual ~Schedule() = default;

	String getId() const;
	String getDate() const;
	String getStartTime() const;
	String getEndTime() const;
	int getBreakMinutes() const;
	String getBranchId() const;
	int getRequiredEmployees() const;
	const Vector<String>& getEmployeeIds() const;
	int getAssignedEmployeeCount() const;
	bool hasCoverage() const;
	bool assignEmployee(const String& employeeId);

	// Phương thức thuần ảo (Pure Virtual)
	virtual bool isOpenShift() const = 0;

	bool operator==(const Schedule& other) const { return id == other.id; }
	bool operator!=(const Schedule& other) const { return id != other.id; }
};

class WorkSchedule : public Schedule {
public:
	using Schedule::Schedule;
};

class FullTimeSchedule : public WorkSchedule {
public:
	FullTimeSchedule(const String& scheduleId, const String& scheduleDate, const String& start,
		const String& end, int breakTime, const String& branch, int required = 1);
	bool isOpenShift() const override;
};

class SelfRegisteredSchedule : public WorkSchedule {
private:
	bool openShift;

public:
	SelfRegisteredSchedule(const String& scheduleId, const String& scheduleDate, const String& start,
		const String& end, int breakTime, const String& branch, int required = 1,
		bool isOpen = true);
	bool isOpenShift() const override;
	bool canRegister() const;
};

class OperatingSchedule {
private:
	String date;
	bool holiday;
	double holidayMultiplier;

public:
	OperatingSchedule(const String& operatingDate, bool isHoliday = false,
		double multiplier = 1.0);
	String getDate() const;
	bool isHoliday() const;
	double salaryModifier() const;
};
