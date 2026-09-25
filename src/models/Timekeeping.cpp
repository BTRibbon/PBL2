#include "Timekeeping.h"

Timekeeping::Timekeeping(std::string employee) : employeeId(employee) {}

std::string Timekeeping::getEmployeeId() const { return employeeId; }

void Timekeeping::addRecord(const AttendanceRecord& record) {
	if (record.employeeId == employeeId) records.push_back(record);
}

const std::vector<AttendanceRecord>& Timekeeping::getRecords() const { return records; }