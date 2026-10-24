#pragma once

#include <cstddef>
#include <initializer_list>

// Các hàm tiện ích toán học cơ bản (thay thế std::min, std::max, std::clamp trong <algorithm>)
template <typename T>
inline T minVal(T a, T b) {
	return (a < b) ? a : b;
}

template <typename T>
inline T maxVal(T a, T b) {
	return (a > b) ? a : b;
}

template <typename T>
inline T clampVal(T val, T low, T high) {
	return (val < low) ? low : ((val > high) ? high : val);
}

// Cấu trúc dữ liệu Mảng động tổng quát tự cài đặt (Rule of Five)
// Quản lý bộ nhớ cấp phát động T* theo nguyên tắc Rule of Five
template <typename T>
class Vector {
private:
	T* data_;
	int size_;
	int capacity_;

public:
	Vector() : data_(nullptr), size_(0), capacity_(0) {}

	explicit Vector(int count) : data_(nullptr), size_(0), capacity_(0) {
		if (count > 0) {
			reserve(count);
			for (int i = 0; i < count; ++i) {
				data_[i] = T();
			}
			size_ = count;
		}
	}

	Vector(int count, const T& initialValue) : data_(nullptr), size_(0), capacity_(0) {
		if (count > 0) {
			reserve(count);
			for (int i = 0; i < count; ++i) {
				data_[i] = initialValue;
			}
			size_ = count;
		}
	}

	Vector(std::initializer_list<T> init) : data_(nullptr), size_(0), capacity_(0) {
		int count = static_cast<int>(init.size());
		if (count > 0) {
			reserve(count);
			int idx = 0;
			for (const T& item : init) {
				data_[idx++] = item;
			}
			size_ = count;
		}
	}

	Vector(const Vector& other) : data_(nullptr), size_(0), capacity_(0) {
		if (other.size_ > 0) {
			reserve(other.size_);
			for (int i = 0; i < other.size_; ++i) {
				data_[i] = other.data_[i];
			}
			size_ = other.size_;
		}
	}

	Vector(Vector&& other) noexcept
		: data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
		other.data_ = nullptr;
		other.size_ = 0;
		other.capacity_ = 0;
	}

	~Vector() {
		delete[] data_;
	}

	Vector& operator=(const Vector& other) {
		if (this == &other) return *this;
		T* newData = nullptr;
		int newCap = other.size_;
		if (newCap > 0) {
			newData = new T[newCap];
			for (int i = 0; i < other.size_; ++i) {
				newData[i] = other.data_[i];
			}
		}
		delete[] data_;
		data_ = newData;
		size_ = other.size_;
		capacity_ = newCap;
		return *this;
	}

	Vector& operator=(Vector&& other) noexcept {
		if (this == &other) return *this;
		delete[] data_;
		data_ = other.data_;
		size_ = other.size_;
		capacity_ = other.capacity_;
		other.data_ = nullptr;
		other.size_ = 0;
		other.capacity_ = 0;
		return *this;
	}

	void reserve(int newCapacity) {
		if (newCapacity <= capacity_) return;
		int targetCap = capacity_ == 0 ? 8 : capacity_ * 2;
		if (targetCap < newCapacity) targetCap = newCapacity;

		T* newData = new T[targetCap];
		for (int i = 0; i < size_; ++i) {
			newData[i] = data_[i];
		}
		delete[] data_;
		data_ = newData;
		capacity_ = targetCap;
	}

	void resize(int newSize, const T& defaultVal = T()) {
		if (newSize < 0) newSize = 0;
		if (newSize > capacity_) {
			reserve(newSize);
		}
		for (int i = size_; i < newSize; ++i) {
			data_[i] = defaultVal;
		}
		size_ = newSize;
	}

	void push_back(const T& value) {
		T copyVal = value;
		if (size_ + 1 > capacity_) {
			reserve(size_ + 1);
		}
		data_[size_++] = copyVal;
	}

	void pop_back() {
		if (size_ > 0) {
			--size_;
		}
	}

	void clear() {
		size_ = 0;
	}

	void eraseAt(int index) {
		if (index < 0 || index >= size_) return;
		for (int i = index; i < size_ - 1; ++i) {
			data_[i] = data_[i + 1];
		}
		--size_;
	}

	T* erase(T* pos) {
		if (!pos || pos < data_ || pos >= data_ + size_) return end();
		int idx = static_cast<int>(pos - data_);
		eraseAt(idx);
		return data_ + idx;
	}

	void reverse() {
		for (int i = 0, j = size_ - 1; i < j; ++i, --j) {
			T temp = data_[i];
			data_[i] = data_[j];
			data_[j] = temp;
		}
	}

	bool contains(const T& value) const {
		for (int i = 0; i < size_; ++i) {
			if (data_[i] == value) return true;
		}
		return false;
	}

	int size() const { return size_; }
	int capacity() const { return capacity_; }
	bool empty() const { return size_ == 0; }

	T* data() { return data_; }
	const T* data() const { return data_; }

	T& operator[](int index) { return data_[index]; }
	const T& operator[](int index) const { return data_[index]; }

	T& front() { return data_[0]; }
	const T& front() const { return data_[0]; }

	T& back() { return data_[size_ - 1]; }
	const T& back() const { return data_[size_ - 1]; }

	T* begin() { return data_; }
	T* end() { return data_ + size_; }
	const T* begin() const { return data_; }
	const T* end() const { return data_ + size_; }
};
