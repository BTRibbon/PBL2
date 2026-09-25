#include "ShiftManager.h"

#include <string>

void ShiftManager::notifyMissingCoverage(const std::string& scheduleId, int missingEmployees) {
	alerts.push_back("Schedule " + scheduleId + " is missing " + std::to_string(missingEmployees) + " employee(s).");
}

const std::vector<std::string>& ShiftManager::getAlerts() const { return alerts; }