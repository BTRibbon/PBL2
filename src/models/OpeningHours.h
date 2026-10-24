#pragma once

#include "../datastructures/String.h"

class OpeningHours {
private:
	String openTime;
	String closeTime;

public:
	OpeningHours(const String& open = "08:00", const String& close = "22:00");
	void setHours(const String& open, const String& close);
	String getOpenTime() const;
	String getCloseTime() const;

	bool operator==(const OpeningHours& other) const {
		return openTime == other.openTime && closeTime == other.closeTime;
	}
	bool operator!=(const OpeningHours& other) const {
		return !(*this == other);
	}
};
