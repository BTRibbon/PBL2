#pragma once

#include <string>
#include <utility>
#include <vector>

struct Order {
	int id;
	int total;
	std::string branch;
	std::string orderId;
	std::string customerId;
	std::vector<std::pair<std::string,int>> items;
	std::string status = "pending";
};
