#include "ShiftManager.h"

#include <string>

void ShiftManager::clearAlerts() {
	alerts.clear();
}

void ShiftManager::notifyMissingCoverage(const std::string& scheduleId, int missingEmployees, const std::string& branchId) {
	if (!branchId.empty()) {
		alerts.push_back("Ca " + scheduleId + " (" + branchId + ") đang thiếu " + std::to_string(missingEmployees) + " nhân sự.");
	} else {
		alerts.push_back("Ca " + scheduleId + " đang thiếu " + std::to_string(missingEmployees) + " nhân sự.");
	}
}

const std::vector<std::string>& ShiftManager::getAlerts() const { return alerts; }
