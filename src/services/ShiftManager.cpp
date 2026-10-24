#include "ShiftManager.h"

void ShiftManager::clearAlerts() {
	alerts.clear();
}

void ShiftManager::notifyMissingCoverage(const String& scheduleId, int missingEmployees, const String& branchId) {
	if (!branchId.empty()) {
		alerts.push_back("Ca " + scheduleId + " (" + branchId + ") đang thiếu " + toString(missingEmployees) + " nhân sự.");
	} else {
		alerts.push_back("Ca " + scheduleId + " đang thiếu " + toString(missingEmployees) + " nhân sự.");
	}
}

const Vector<String>& ShiftManager::getAlerts() const { return alerts; }
