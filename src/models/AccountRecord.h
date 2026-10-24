#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include "../datastructures/String.h"
#include "../datastructures/Vector.h"
#include "Employee.h"

struct AccountRecord {
	String id;
	String username;
	String name;
	String role;
	String branchId;
	String salt;
	String passwordHash;
	Employee* user = nullptr;

	AccountRecord() = default;
	AccountRecord(const String& accId, const String& accUser, const String& accName,
		const String& accRole, const String& accBranch, const String& accSalt,
		const String& accHash, Employee* empPtr = nullptr)
		: id(accId), username(accUser), name(accName), role(accRole),
		  branchId(accBranch), salt(accSalt), passwordHash(accHash), user(empPtr) {}
};

// Hàm băm mật khẩu FNV-1a 64-bit kết hợp chuỗi muối (Salt)
inline String HashPassword(const String& password, const String& salt) {
	const String input = salt + ":" + password;
	std::uint64_t hash = 14695981039346656037ULL;
	for (std::size_t i = 0; i < input.size(); ++i) {
		hash ^= static_cast<unsigned char>(input[i]);
		hash *= 1099511628211ULL;
	}
	char hexBuf[24];
	std::snprintf(hexBuf, sizeof(hexBuf), "%016llx", static_cast<unsigned long long>(hash));
	return String(hexBuf);
}

inline String GenerateSalt() {
	static const char hexChars[] = "0123456789abcdef";
	String salt;
	salt.reserve(32);
	for (int i = 0; i < 32; ++i) {
		salt.push_back(hexChars[std::rand() % 16]);
	}
	return salt;
}
