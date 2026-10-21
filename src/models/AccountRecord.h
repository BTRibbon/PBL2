#pragma once

#include <string>
#include <cstdint>
#include <sstream>
#include <fstream>
#include <vector>
#include <memory>
#include <cstdlib>
#include "User.h"

struct AccountRecord {
	std::string id;
	std::string username;
	std::string name;
	std::string role;
	std::string branchId;
	std::string salt;
	std::string passwordHash;
	User* user = nullptr;
};

inline std::string HashPassword(const std::string& password, const std::string& salt) {
	const std::string input = salt + ":" + password;
	std::uint64_t hash = 14695981039346656037ULL;
	for (unsigned char character : input) {
		hash ^= character;
		hash *= 1099511628211ULL;
	}
	std::ostringstream result;
	result << std::hex;
	result.width(16);
	result.fill('0');
	result << hash;
	return result.str();
}

inline std::string GenerateSalt() {
	static const char hexChars[] = "0123456789abcdef";
	std::string salt;
	salt.reserve(32);
	for (int i = 0; i < 32; ++i) {
		salt += hexChars[std::rand() % 16];
	}
	return salt;
}

inline bool SaveAccountsToFile(const std::vector<AccountRecord>& accounts) {
	std::ostringstream plain;
	for (const auto& acc : accounts) {
		plain << acc.id << "|" << acc.username << "|" << acc.name << "|"
		      << acc.role << "|" << acc.branchId << "|" << acc.salt << "|"
		      << acc.passwordHash << "\n";
	}
	std::string plainStr = plain.str();
	std::string encrypted;
	encrypted.reserve(plainStr.size());
	for (char c : plainStr) {
		encrypted.push_back(static_cast<char>(static_cast<unsigned char>(c) ^ 0x5A));
	}
	std::ofstream binFile("data/accounts.dat", std::ios::binary);
	if (!binFile) return false;
	binFile.write(encrypted.data(), encrypted.size());
	return true;
}

