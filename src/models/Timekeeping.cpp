#include "Timekeeping.h"

Timekeeping::Timekeeping(const String& employee) : employeeId(employee) {}

String Timekeeping::getEmployeeId() const { return employeeId; }

void Timekeeping::addRecord(const AttendanceRecord& record) {
	if (record.employeeId == employeeId) records.push_back(record);
}

bool Timekeeping::checkIn(const String& scheduleId, const String& timestamp) {
	for (int i = 0; i < records.size(); ++i) {
		if (records[i].scheduleId == scheduleId && records[i].checkOut.empty()) {
			return false;
		}
	}
	records.push_back(AttendanceRecord(scheduleId, employeeId, timestamp, "", true));
	return true;
}

bool Timekeeping::checkOut(const String& scheduleId, const String& timestamp) {
	for (int i = records.size() - 1; i >= 0; --i) {
		if (records[i].scheduleId == scheduleId && records[i].checkOut.empty()) {
			records[i].checkOut = timestamp;
			return true;
		}
	}
	return false;
}

const Vector<AttendanceRecord>& Timekeeping::getRecords() const { return records; }
