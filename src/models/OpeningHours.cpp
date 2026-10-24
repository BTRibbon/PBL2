#include "OpeningHours.h"

OpeningHours::OpeningHours(const String& open, const String& close)
	: openTime(open), closeTime(close) {}

void OpeningHours::setHours(const String& open, const String& close) {
	openTime = open;
	closeTime = close;
}

String OpeningHours::getOpenTime() const { return openTime; }

String OpeningHours::getCloseTime() const { return closeTime; }
