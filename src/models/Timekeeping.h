#pragma once

#include <string>
#include <vector>

struct AttendanceRecord {
	std::string scheduleId;
	std::string employeeId;
	std::string checkIn;
	std::string checkOut;
	bool isPresent = false;
};

class Timekeeping {
	std::string employeeId;
	std::vector<AttendanceRecord> records;

public:
	Timekeeping(std::string employee);
	std::string getEmployeeId() const;
	void addRecord(const AttendanceRecord& record);
	const std::vector<AttendanceRecord>& getRecords() const;
};