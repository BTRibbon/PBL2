#pragma once

#include "raylib.h"
#include "../datastructures/String.h"
#include "../datastructures/Vector.h"
#include "../datastructures/MinHeap.h"
#include "../models/Branch.h"
#include "../models/RestaurantChain.h"
#include "../models/Employee.h"

struct AccountRecord;

struct MapGridPoint {
	int x = 0;
	int y = 0;
};

struct DutRoute {
	Vector<int> nodes;
	Vector<int> explored;
	float distance = 1e9f;
};

enum class DutCellType {
	Road,
	MainHighway,
	Campus,
	CampusBuilding,
	Market,
	Water,
	Building,
	Park
};

class DutMap {
public:
	static constexpr int COLS = 30;
	static constexpr int ROWS = 20;

private:
	Vector<DutCellType> cells;
	Vector<bool> blocked;

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
		x = clampVal(x, 0, COLS - 1);
		y = clampVal(y, 0, ROWS - 1);
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

	Vector<int> neighbours(int node, int target = -1) const;
	DutRoute findRoute(int start, int target, bool aStar) const;
};

class DutMapUI {
public:
	DutMap map;
	float mapOriginX = 245.0f;
	float mapOriginY = 168.0f;
	float mapCellSize = 24.0f;

	int selectionStep = 1;
	int startBranchIdx = 0;
	int targetBranchIdx = 1;
	MapGridPoint targetPoint{12, 13};
	bool isTargetCustom = false;

	DutRoute currentRoute;
	bool useAStar = true;
	float pathProgress = 0.0f;
	float shipperProgress = 0.0f;

	// Hộp thoại Quản lý chi nhánh (dành cho Giám đốc chuỗi - ChainManager)
	bool isBranchModalOpen = false;
	int branchModalMode = 0; // 0: Thêm, 1: Sửa
	int editingBranchIdx = -1;
	String inputBranchId;
	String inputBranchName;
	int inputBranchGridX = 12;
	int inputBranchGridY = 3;
	int branchModalField = 0;
	int branchListScroll = 0;

	int hoveredCellX = -1;
	int hoveredCellY = -1;

	DutMapUI();

	void updateRoute();
	bool handleClick(Vector2 mouse, Vector<Branch>& branches, RestaurantChain& chain,
		Vector<AccountRecord>& accounts, Employee* currentUser,
		String& statusMessage, int& statusVersion);
	void handleScroll(Vector2 mouse, float wheelMove);
	bool handleInput(int key, int charPressed, Vector<Branch>& branches, RestaurantChain& chain,
		Vector<AccountRecord>& accounts, Employee* currentUser,
		String& statusMessage, int& statusVersion);

	void draw(Vector2 mouse, const Vector<Branch>& branches, Employee* currentUser);

	bool isModalOpen() const { return isBranchModalOpen; }
	void openAddBranchModal(const Vector<Branch>& branches);
	void openEditBranchModal(int branchIdx, const Vector<Branch>& branches);
	bool submitBranchModal(Vector<Branch>& branches, RestaurantChain& chain,
		Vector<AccountRecord>& accounts, String& statusMessage, int& statusVersion);

	void deleteBranch(int branchIdx, Vector<Branch>& branches, RestaurantChain& chain,
		Vector<AccountRecord>& accounts, String& statusMessage, int& statusVersion);
};
