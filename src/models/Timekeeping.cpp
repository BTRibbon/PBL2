#include "Timekeeping.h"

Timekeeping::Timekeeping(std::string employee) : employeeId(employee) {}

std::string Timekeeping::getEmployeeId() const { return employeeId; }

void Timekeeping::addRecord(const AttendanceRecord& record) {
	if (record.employeeId == employeeId) records.push_back(record);
}

bool Timekeeping::checkIn(const std::string& scheduleId, const std::string& timestamp) {
	for (const AttendanceRecord& record : records)
		if (record.scheduleId == scheduleId && record.checkOut.empty()) return false;
	records.push_back({scheduleId, employeeId, timestamp, "", true});
	return true;
}

bool Timekeeping::checkOut(const std::string& scheduleId, const std::string& timestamp) {
	for (auto it = records.rbegin(); it != records.rend(); ++it) {
		if (it->scheduleId == scheduleId && it->checkOut.empty()) {
			it->checkOut = timestamp;
			return true;
		}
	}
	return false;
}

const std::vector<AttendanceRecord>& Timekeeping::getRecords() const { return records; }