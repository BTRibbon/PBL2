#include "String.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

void String::allocateAndCopy(const char* src, std::size_t len) {
	length_ = len;
	capacity_ = len < 15 ? 15 : len;
	data_ = new char[capacity_ + 1];
	if (src && len > 0) {
		std::memcpy(data_, src, len);
	}
	data_[length_] = '\0';
}

String::String() : data_(nullptr), length_(0), capacity_(0) {
	allocateAndCopy("", 0);
}

String::String(const char* str) : data_(nullptr), length_(0), capacity_(0) {
	if (!str) {
		allocateAndCopy("", 0);
	} else {
		allocateAndCopy(str, std::strlen(str));
	}
}

String::String(const char* str, std::size_t len) : data_(nullptr), length_(0), capacity_(0) {
	if (!str || len == 0) {
		allocateAndCopy("", 0);
	} else {
		allocateAndCopy(str, len);
	}
}

String::String(std::size_t count, char ch) : data_(nullptr), length_(0), capacity_(0) {
	length_ = count;
	capacity_ = count < 15 ? 15 : count;
	data_ = new char[capacity_ + 1];
	for (std::size_t i = 0; i < count; ++i) {
		data_[i] = ch;
	}
	data_[length_] = '\0';
}

String::String(const String& other) : data_(nullptr), length_(0), capacity_(0) {
	allocateAndCopy(other.data_, other.length_);
}

String::String(String&& other) noexcept
	: data_(other.data_), length_(other.length_), capacity_(other.capacity_) {
	other.data_ = new char[1];
	other.data_[0] = '\0';
	other.length_ = 0;
	other.capacity_ = 0;
}

String::~String() {
	delete[] data_;
}

String& String::operator=(const String& other) {
	if (this == &other) return *this;
	char* newData = new char[(other.length_ < 15 ? 15 : other.length_) + 1];
	if (other.length_ > 0) {
		std::memcpy(newData, other.data_, other.length_);
	}
	newData[other.length_] = '\0';
	delete[] data_;
	data_ = newData;
	length_ = other.length_;
	capacity_ = other.length_ < 15 ? 15 : other.length_;
	return *this;
}

String& String::operator=(String&& other) noexcept {
	if (this == &other) return *this;
	delete[] data_;
	data_ = other.data_;
	length_ = other.length_;
	capacity_ = other.capacity_;
	other.data_ = new char[1];
	other.data_[0] = '\0';
	other.length_ = 0;
	other.capacity_ = 0;
	return *this;
}

String& String::operator=(const char* str) {
	if (!str) {
		clear();
		return *this;
	}
	std::size_t len = std::strlen(str);
	char* newData = new char[(len < 15 ? 15 : len) + 1];
	if (len > 0) {
		std::memcpy(newData, str, len);
	}
	newData[len] = '\0';
	delete[] data_;
	data_ = newData;
	length_ = len;
	capacity_ = len < 15 ? 15 : len;
	return *this;
}

void String::clear() {
	length_ = 0;
	if (data_) data_[0] = '\0';
}

void String::reserve(std::size_t newCapacity) {
	if (newCapacity <= capacity_) return;
	std::size_t targetCap = capacity_ * 2;
	if (targetCap < newCapacity) targetCap = newCapacity;
	if (targetCap < 15) targetCap = 15;

	char* newData = new char[targetCap + 1];
	if (length_ > 0 && data_) {
		std::memcpy(newData, data_, length_);
	}
	newData[length_] = '\0';
	delete[] data_;
	data_ = newData;
	capacity_ = targetCap;
}

void String::push_back(char ch) {
	if (length_ + 1 > capacity_) {
		reserve(length_ + 1);
	}
	data_[length_++] = ch;
	data_[length_] = '\0';
}

void String::pop_back() {
	if (length_ > 0) {
		--length_;
		data_[length_] = '\0';
	}
}

void String::append(const char* str, std::size_t len) {
	if (!str || len == 0) return;
	// Phòng trường hợp str nằm trong vùng nhớ của chính đối tượng này
	if (str >= data_ && str < data_ + length_) {
		String temp(str, len);
		append(temp.data_, temp.length_);
		return;
	}
	if (length_ + len > capacity_) {
		reserve(length_ + len);
	}
	std::memcpy(data_ + length_, str, len);
	length_ += len;
	data_[length_] = '\0';
}

void String::append(const char* str) {
	if (!str) return;
	append(str, std::strlen(str));
}

void String::append(const String& other) {
	append(other.data_, other.length_);
}

void String::insert(std::size_t pos, const char* str) {
	if (!str) return;
	std::size_t insertLen = std::strlen(str);
	if (insertLen == 0) return;
	if (pos > length_) pos = length_;
	if (length_ + insertLen > capacity_) {
		reserve(length_ + insertLen);
	}
	std::memmove(data_ + pos + insertLen, data_ + pos, length_ - pos + 1);
	std::memcpy(data_ + pos, str, insertLen);
	length_ += insertLen;
}

String String::substr(std::size_t pos, std::size_t count) const {
	if (pos >= length_) return String();
	std::size_t avail = length_ - pos;
	std::size_t actualCount = (count == npos || count > avail) ? avail : count;
	return String(data_ + pos, actualCount);
}

std::size_t String::find(const char* needle, std::size_t pos) const {
	if (!needle) return npos;
	std::size_t needleLen = std::strlen(needle);
	if (needleLen == 0) return pos <= length_ ? pos : npos;
	if (pos >= length_ || needleLen > length_ - pos) return npos;
	const char* found = std::strstr(data_ + pos, needle);
	return found ? static_cast<std::size_t>(found - data_) : npos;
}

std::size_t String::find(const String& needle, std::size_t pos) const {
	return find(needle.c_str(), pos);
}

std::size_t String::find(char ch, std::size_t pos) const {
	for (std::size_t i = pos; i < length_; ++i) {
		if (data_[i] == ch) return i;
	}
	return npos;
}

std::size_t String::rfind(const char* needle, std::size_t pos) const {
	if (!needle) return npos;
	std::size_t needleLen = std::strlen(needle);
	if (needleLen > length_) return npos;
	if (needleLen == 0) return pos < length_ ? pos : length_;

	std::size_t maxStart = length_ - needleLen;
	if (pos != npos && pos < maxStart) maxStart = pos;

	for (std::size_t i = maxStart + 1; i > 0; --i) {
		std::size_t idx = i - 1;
		if (std::strncmp(data_ + idx, needle, needleLen) == 0) {
			return idx;
		}
	}
	return npos;
}

bool String::startsWith(const char* prefix) const {
	if (!prefix) return false;
	std::size_t prefixLen = std::strlen(prefix);
	if (prefixLen > length_) return false;
	return std::strncmp(data_, prefix, prefixLen) == 0;
}

String String::toLower() const {
	String result(*this);
	for (std::size_t i = 0; i < result.length_; ++i) {
		result.data_[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result.data_[i])));
	}
	return result;
}

String String::toUpper() const {
	String result(*this);
	for (std::size_t i = 0; i < result.length_; ++i) {
		result.data_[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(result.data_[i])));
	}
	return result;
}

String String::fromInt(int value) {
	char buffer[32];
	std::snprintf(buffer, sizeof(buffer), "%d", value);
	return String(buffer);
}

String String::fromFloat(float value, int decimals) {
	char buffer[64];
	if (decimals < 0) decimals = 0;
	if (decimals > 6) decimals = 6;
	char fmt[16];
	std::snprintf(fmt, sizeof(fmt), "%%.%df", decimals);
	std::snprintf(buffer, sizeof(buffer), fmt, static_cast<double>(value));
	return String(buffer);
}

bool String::tryParseInt(int& outValue) const {
	if (empty()) return false;
	char* endPtr = nullptr;
	long val = std::strtol(data_, &endPtr, 10);
	if (endPtr == data_ || *endPtr != '\0') return false;
	outValue = static_cast<int>(val);
	return true;
}

int String::toInt(int defaultValue) const {
	if (empty()) return defaultValue;
	char* endPtr = nullptr;
	long val = std::strtol(data_, &endPtr, 10);
	if (endPtr == data_) return defaultValue;
	return static_cast<int>(val);
}

float String::toFloat(float defaultValue) const {
	if (empty()) return defaultValue;
	char* endPtr = nullptr;
	float val = std::strtof(data_, &endPtr);
	if (endPtr == data_) return defaultValue;
	return val;
}

String& String::operator+=(const String& other) {
	append(other);
	return *this;
}

String& String::operator+=(const char* str) {
	append(str);
	return *this;
}

String& String::operator+=(char ch) {
	push_back(ch);
	return *this;
}

bool String::operator==(const String& rhs) const {
	if (length_ != rhs.length_) return false;
	return std::memcmp(data_, rhs.data_, length_) == 0;
}

bool String::operator==(const char* rhs) const {
	if (!rhs) return false;
	return std::strcmp(data_, rhs) == 0;
}

bool String::operator!=(const String& rhs) const {
	return !(*this == rhs);
}

bool String::operator!=(const char* rhs) const {
	return !(*this == rhs);
}

bool String::operator<(const String& rhs) const {
	return std::strcmp(data_, rhs.data_) < 0;
}

bool String::operator<=(const String& rhs) const {
	return std::strcmp(data_, rhs.data_) <= 0;
}

bool String::operator>(const String& rhs) const {
	return std::strcmp(data_, rhs.data_) > 0;
}

bool String::operator>=(const String& rhs) const {
	return std::strcmp(data_, rhs.data_) >= 0;
}

bool operator==(const char* lhs, const String& rhs) {
	return rhs == lhs;
}

bool operator!=(const char* lhs, const String& rhs) {
	return !(rhs == lhs);
}

String operator+(const String& lhs, const String& rhs) {
	String result;
	result.reserve(lhs.size() + rhs.size());
	result.append(lhs);
	result.append(rhs);
	return result;
}

String operator+(const String& lhs, const char* rhs) {
	String result;
	std::size_t rhsLen = rhs ? std::strlen(rhs) : 0;
	result.reserve(lhs.size() + rhsLen);
	result.append(lhs);
	if (rhs) result.append(rhs, rhsLen);
	return result;
}

String operator+(const char* lhs, const String& rhs) {
	String result;
	std::size_t lhsLen = lhs ? std::strlen(lhs) : 0;
	result.reserve(lhsLen + rhs.size());
	if (lhs) result.append(lhs, lhsLen);
	result.append(rhs);
	return result;
}

String operator+(const String& lhs, char rhs) {
	String result;
	result.reserve(lhs.size() + 1);
	result.append(lhs);
	result.push_back(rhs);
	return result;
}

