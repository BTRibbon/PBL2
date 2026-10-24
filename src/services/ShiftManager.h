#pragma once

#include "../datastructures/String.h"
#include "../datastructures/Vector.h"

class ShiftManager {
private:
	Vector<String> alerts;

public:
	void clearAlerts();
	void notifyMissingCoverage(const String& scheduleId, int missingEmployees, const String& branchId = "");
	const Vector<String>& getAlerts() const;
};
