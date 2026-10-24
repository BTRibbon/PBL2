#pragma once

#include "../datastructures/String.h"
#include "../datastructures/Vector.h"

struct AttendanceRecord {
	String scheduleId;
	String employeeId;
	String checkIn;
	String checkOut;
	bool isPresent = false;

	AttendanceRecord() = default;
	AttendanceRecord(const String& schedId, const String& empId,
		const String& inTime, const String& outTime, bool present)
		: scheduleId(schedId), employeeId(empId),
		  checkIn(inTime), checkOut(outTime), isPresent(present) {}
};

class Timekeeping {
private:
	String employeeId;
	Vector<AttendanceRecord> records;

public:
	Timekeeping() = default;
	explicit Timekeeping(const String& employee);
	String getEmployeeId() const;
	void addRecord(const AttendanceRecord& record);
	bool checkIn(const String& scheduleId, const String& timestamp);
	bool checkOut(const String& scheduleId, const String& timestamp);
	const Vector<AttendanceRecord>& getRecords() const;
};
