#include "OpeningHours.h"

OpeningHours::OpeningHours(std::string open,std::string close) : openTime(open),closeTime(close) {}

void OpeningHours::setHours(std::string open,std::string close) { openTime=open;closeTime=close; }

std::string OpeningHours::getOpenTime() const { return openTime; }

std::string OpeningHours::getCloseTime() const { return closeTime; }
