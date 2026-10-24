#pragma once

#include <cstddef>

// Lớp chuỗi ký tự động tự cài đặt quản lý bộ nhớ cấp phát động char* (Rule of Five)
// Quản lý mảng ký tự cấp phát động (char*) theo nguyên tắc Rule of Five
class String {
private:
	char* data_;
	std::size_t length_;
	std::size_t capacity_;

	void allocateAndCopy(const char* src, std::size_t len);

public:
	static constexpr std::size_t npos = static_cast<std::size_t>(-1);

	// Constructors & Destructor (Rule of Five)
	String();
	String(const char* str);
	String(const char* str, std::size_t len);
	String(std::size_t count, char ch);
	String(const String& other);
	String(String&& other) noexcept;
	~String();

	// Toán tử gán (Copy & Move Assignment)
	String& operator=(const String& other);
	String& operator=(String&& other) noexcept;
	String& operator=(const char* str);

	// Truy cập dữ liệu
	const char* c_str() const { return data_; }
	const char* data() const { return data_; }
	char* data() { return data_; }
	std::size_t size() const { return length_; }
	std::size_t length() const { return length_; }
	std::size_t capacity() const { return capacity_; }
	bool empty() const { return length_ == 0; }

	char& operator[](std::size_t index) { return data_[index]; }
	const char& operator[](std::size_t index) const { return data_[index]; }
	char back() const { return length_ > 0 ? data_[length_ - 1] : '\0'; }

	// Iterators phục vụ vòng lặp range-based for
	char* begin() { return data_; }
	char* end() { return data_ + length_; }
	const char* begin() const { return data_; }
	const char* end() const { return data_ + length_; }

	// Thao tác chỉnh sửa chuỗi
	void clear();
	void reserve(std::size_t newCapacity);
	void push_back(char ch);
	void pop_back();
	void append(const char* str, std::size_t len);
	void append(const char* str);
	void append(const String& other);
	void insert(std::size_t pos, const char* str);

	// Cắt chuỗi & tìm kiếm
	String substr(std::size_t pos, std::size_t count = npos) const;
	std::size_t find(const char* needle, std::size_t pos = 0) const;
	std::size_t find(const String& needle, std::size_t pos = 0) const;
	std::size_t find(char ch, std::size_t pos = 0) const;
	std::size_t rfind(const char* needle, std::size_t pos = npos) const;
	bool startsWith(const char* prefix) const;

	// Chuyển đổi chữ hoa / chữ thường
	String toLower() const;
	String toUpper() const;

	// Chuyển đổi số <-> chuỗi
	static String fromInt(int value);
	static String fromFloat(float value, int decimals = 1);
	bool tryParseInt(int& outValue) const;
	int toInt(int defaultValue = 0) const;
	float toFloat(float defaultValue = 0.0f) const;

	// Nạp chồng toán tử nối chuỗi
	String& operator+=(const String& other);
	String& operator+=(const char* str);
	String& operator+=(char ch);

	// Nạp chồng toán tử so sánh
	bool operator==(const String& rhs) const;
	bool operator==(const char* rhs) const;
	bool operator!=(const String& rhs) const;
	bool operator!=(const char* rhs) const;
	bool operator<(const String& rhs) const;
	bool operator<=(const String& rhs) const;
	bool operator>(const String& rhs) const;
	bool operator>=(const String& rhs) const;
};

// Toán tử tự do (Non-member operators)
bool operator==(const char* lhs, const String& rhs);
bool operator!=(const char* lhs, const String& rhs);
String operator+(const String& lhs, const String& rhs);
String operator+(const String& lhs, const char* rhs);
String operator+(const char* lhs, const String& rhs);
String operator+(const String& lhs, char rhs);

inline String toString(int value) { return String::fromInt(value); }
