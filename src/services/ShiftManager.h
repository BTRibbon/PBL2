#pragma once

#include <string>
#include <vector>

class ShiftManager {
	std::vector<std::string> alerts;

public:
	void clearAlerts();
	void notifyMissingCoverage(const std::string& scheduleId, int missingEmployees, const std::string& branchId = "");
	const std::vector<std::string>& getAlerts() const;
};
