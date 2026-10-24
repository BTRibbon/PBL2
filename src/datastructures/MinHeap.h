#pragma once

#include "Vector.h"

// Cấu trúc dữ liệu Hàng đợi ưu tiên Min-Heap tự cài đặt bằng cây nhị phân mảng động
// Phục vụ giải thuật tìm đường ngắn nhất Dijkstra & A* với độ phức tạp O(log V)
template <typename T>
class MinHeap {
private:
	Vector<T> heap_;

	void siftUp(int index) {
		while (index > 0) {
			int parent = (index - 1) / 2;
			if (heap_[index] < heap_[parent]) {
				T temp = heap_[index];
				heap_[index] = heap_[parent];
				heap_[parent] = temp;
				index = parent;
			} else {
				break;
			}
		}
	}

	void siftDown(int index) {
		int n = heap_.size();
		while (true) {
			int left = 2 * index + 1;
			int right = 2 * index + 2;
			int smallest = index;

			if (left < n && heap_[left] < heap_[smallest]) {
				smallest = left;
			}
			if (right < n && heap_[right] < heap_[smallest]) {
				smallest = right;
			}
			if (smallest != index) {
				T temp = heap_[index];
				heap_[index] = heap_[smallest];
				heap_[smallest] = temp;
				index = smallest;
			} else {
				break;
			}
		}
	}

public:
	MinHeap() = default;

	bool empty() const { return heap_.empty(); }
	int size() const { return heap_.size(); }
	void clear() { heap_.clear(); }

	void push(const T& item) {
		heap_.push_back(item);
		siftUp(heap_.size() - 1);
	}

	const T& top() const {
		return heap_.front();
	}

	T pop() {
		T result = heap_.front();
		heap_[0] = heap_.back();
		heap_.pop_back();
		if (!heap_.empty()) {
			siftDown(0);
		}
		return result;
	}
};

