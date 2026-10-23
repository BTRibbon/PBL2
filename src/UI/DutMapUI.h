#pragma once

#include "raylib.h"
#include "../models/Branch.h"
#include "../models/RestaurantChain.h"
#include "../models/User.h"
#include "../datastructures/OrderQueue.h"
#include <algorithm>
#include <string>
#include <vector>
#include <memory>
#include <queue>

struct AccountRecord;

struct MapGridPoint {
	int x;
	int y;
};

struct DutRoute {
	std::vector<int> nodes;
	std::vector<int> explored;
	float distance = 1e9f;
};

enum class DutCellType {
	Road,          // Đường nhựa nhánh
	MainHighway,    // Trục chính Nguyễn Lương Bằng (QL1A)
	Campus,        // Khuôn viên ĐHBK Đà Nẵng
	CampusBuilding,// Toà nhà ĐHBK (Khu A, Khu B, Thư viện, Sân bóng)
	Market,        // Chợ Hoà Khánh
	Water,         // Hồ Bàu Tràm
	Building,      // Nhà dân, quán xá
	Park           // Cây xanh, công viên
};

class DutMap {
public:
	static constexpr int COLS = 30;
	static constexpr int ROWS = 20;

private:
	std::vector<DutCellType> cells;
	std::vector<bool> blocked;

	int id(int x, int y) const { return y * COLS + x; }

public:
	DutMap();
	bool isInside(int x, int y) const { return x >= 0 && x < COLS && y >= 0 && y < ROWS; }
	bool isBlocked(int x, int y) const { return insideBlocked(x, y); }
	bool insideBlocked(int x, int y) const {
		if (!isInside(x, y)) return true;
		return blocked[id(x, y)];
	}
	DutCellType getCellType(int x, int y) const {
		if (!isInside(x, y)) return DutCellType::Building;
		return cells[id(x, y)];
	}
	int nodeAt(int x, int y) const {
		x = std::clamp(x, 0, COLS - 1);
		y = std::clamp(y, 0, ROWS - 1);
		return id(x, y);
	}
	int getNodeX(int node) const {
		if (node < 0 || node >= COLS * ROWS) return 0;
		return node % COLS;
	}
	int getNodeY(int node) const {
		if (node < 0 || node >= COLS * ROWS) return 0;
		return node / COLS;
	}

	std::vector<int> neighbours(int node, int target = -1) const;
	DutRoute findRoute(int start, int target, bool aStar) const;
};

class DutMapUI {
public:
	DutMap map;
	float mapOriginX = 245.0f;
	float mapOriginY = 168.0f;
	float mapCellSize = 24.0f;

	// Selection state: 2-click mechanism
	// Step 0: waiting for Start branch
	// Step 1: Start chosen, waiting for Destination (branch or custom address)
	int selectionStep = 1;
	int startBranchIdx = 0;
	int targetBranchIdx = 1;
	MapGridPoint targetPoint{12, 13};
	bool isTargetCustom = false;

	// Path calculation & animation
	DutRoute currentRoute;
	bool useAStar = true;
	float pathProgress = 0.0f;
	float shipperProgress = 0.0f; // 0.0 -> route length for vehicle animation

	// Branch Management Modal (BranchManager only)
	bool isBranchModalOpen = false;
	int branchModalMode = 0; // 0: Add, 1: Edit
	int editingBranchIdx = -1;
	std::string inputBranchId;
	std::string inputBranchName;
	int inputBranchGridX = 12;
	int inputBranchGridY = 3;
	int branchModalField = 0;
	int branchListScroll = 0;

	// Hover info
	int hoveredCellX = -1;
	int hoveredCellY = -1;

	DutMapUI();

	void updateRoute();
	bool handleClick(Vector2 mouse, std::vector<Branch>& branches,
		std::vector<AccountRecord>& accounts, std::vector<std::unique_ptr<User>>& userPool,
		User* currentUser, std::string& statusMessage, int& statusVersion);
	void handleScroll(Vector2 mouse, float wheelMove);
	bool handleInput(int key, int charPressed, std::vector<Branch>& branches,
		std::vector<AccountRecord>& accounts, std::vector<std::unique_ptr<User>>& userPool,
		User* currentUser, std::string& statusMessage, int& statusVersion);

	void draw(Vector2 mouse, const std::vector<Branch>& branches,
		OrderQueue& orderQueue, User* currentUser);

	bool isModalOpen() const { return isBranchModalOpen; }
	void openAddBranchModal(const std::vector<Branch>& branches);
	void openEditBranchModal(int branchIdx, const std::vector<Branch>& branches);
	bool submitBranchModal(std::vector<Branch>& branches,
		std::vector<AccountRecord>& accounts, std::vector<std::unique_ptr<User>>& userPool,
		std::string& statusMessage, int& statusVersion);

	void deleteBranch(int branchIdx, std::vector<Branch>& branches,
		std::vector<AccountRecord>& accounts, std::vector<std::unique_ptr<User>>& userPool,
		std::string& statusMessage, int& statusVersion);
};
