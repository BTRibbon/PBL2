#pragma once

#include <string>

class OpeningHours {
	std::string openTime;
	std::string closeTime;
public:
	OpeningHours(std::string open="08:00",std::string close="22:00");
	void setHours(std::string open,std::string close);
	std::string getOpenTime() const;
	std::string getCloseTime() const;
};
