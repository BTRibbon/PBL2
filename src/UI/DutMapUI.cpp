#include "DutMapUI.h"
#include "UIFont.h"
#include "UIAnim.h"
#include "../models/AccountRecord.h"
#include "../services/DataManager.h"
#include "rlgl.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {
struct RouteQueueEntry {
	float fCost;
	int vertex;

	bool operator<(const RouteQueueEntry& other) const {
		return fCost < other.fCost;
	}
};
}

DutMap::DutMap() : cells(COLS * ROWS, DutCellType::Building), blocked(COLS * ROWS, true) {
	auto setCell = [this](int x, int y, DutCellType type, bool isWalkable) {
		if (!isInside(x, y)) return;
		int idx = id(x, y);
		cells[idx] = type;
		blocked[idx] = !isWalkable;
	};

	auto setRect = [setCell](int x1, int y1, int x2, int y2, DutCellType type, bool isWalkable) {
		for (int y = y1; y <= y2; ++y) {
			for (int x = x1; x <= x2; ++x) {
				setCell(x, y, type, isWalkable);
			}
		}
	};

	// 1. Trục chính: Đại lộ Nguyễn Lương Bằng (QL1A)
	for (int y = 0; y < ROWS; ++y) {
		setCell(11, y, DutCellType::MainHighway, true);
		setCell(12, y, DutCellType::MainHighway, true);
	}

	// 2. Đường Ngô Sĩ Liên
	for (int x = 11; x < COLS; ++x) {
		setCell(x, 5, DutCellType::Road, true);
	}

	// 3. Đường Âu Cơ
	for (int x = 0; x <= 12; ++x) {
		setCell(x, 13, DutCellType::Road, true);
	}

	// 4. Đường Lạc Long Quân
	for (int y = 0; y <= 14; ++y) {
		setCell(4, y, DutCellType::Road, true);
	}

	// 5. Đường Đồng Kè
	for (int x = 4; x <= 12; ++x) {
		setCell(x, 9, DutCellType::Road, true);
	}

	// 6. Đường Nam Cao
	for (int x = 11; x <= 25; ++x) {
		setCell(x, 18, DutCellType::Road, true);
	}

	// 7. Đường Mộc Bài
	for (int y = 5; y <= 18; ++y) {
		setCell(19, y, DutCellType::Road, true);
	}

	// 8. Đường Kiệt số 6 & Đoàn Phú Tứ
	for (int x = 4; x <= 11; ++x) {
		setCell(x, 2, DutCellType::Road, true);
		setCell(x, 16, DutCellType::Road, true);
	}

	// 9. Lối đi nội bộ & Cổng chính ĐH Bách Khoa (54 Nguyễn Lương Bằng)
	for (int x = 12; x <= 24; ++x) {
		setCell(x, 3, DutCellType::Road, true);
		setCell(x, 1, DutCellType::Road, true);
	}
	for (int y = 1; y <= 5; ++y) {
		setCell(16, y, DutCellType::Road, true);
		setCell(22, y, DutCellType::Road, true);
	}

	// 10. Đường ven hồ Bàu Tràm
	for (int y = 14; y <= 18; ++y) {
		setCell(24, y, DutCellType::Road, true);
	}

	// 11. Khuôn viên Trường ĐH Bách Khoa - ĐH Đà Nẵng (DUT)
	setRect(13, 0, 25, 4, DutCellType::Campus, false);
	for (int x = 12; x <= 24; ++x) { setCell(x, 3, DutCellType::Road, true); setCell(x, 1, DutCellType::Road, true); }
	for (int y = 1; y <= 5; ++y) { setCell(16, y, DutCellType::Road, true); setCell(22, y, DutCellType::Road, true); }
	setCell(14, 2, DutCellType::CampusBuilding, false);
	setCell(15, 2, DutCellType::CampusBuilding, false);
	setCell(18, 2, DutCellType::CampusBuilding, false);
	setCell(19, 2, DutCellType::CampusBuilding, false);
	setCell(23, 2, DutCellType::CampusBuilding, false);
	setCell(24, 2, DutCellType::CampusBuilding, false);

	// 12. Chợ Hoà Khánh
	setRect(7, 11, 10, 12, DutCellType::Market, false);
	setCell(9, 12, DutCellType::Road, true);

	// 13. Hồ Bàu Tràm
	setRect(26, 14, 29, 19, DutCellType::Water, false);
	setRect(25, 16, 25, 17, DutCellType::Water, false);

	// 14. Công viên & Cây xanh
	setRect(5, 4, 8, 7, DutCellType::Park, false);
	setRect(14, 7, 17, 8, DutCellType::Park, false);
}

Vector<int> DutMap::neighbours(int node, int target) const {
	if (node < 0 || node >= COLS * ROWS) return Vector<int>();
	int x = node % COLS;
	int y = node / COLS;
	Vector<int> result;
	const MapGridPoint dirs[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	for (int i = 0; i < 4; ++i) {
		int nx = x + dirs[i].x;
		int ny = y + dirs[i].y;
		if (isInside(nx, ny)) {
			int nid = id(nx, ny);
			if (!blocked[nid] || nid == target) {
				result.push_back(nid);
			}
		}
	}
	return result;
}

DutRoute DutMap::findRoute(int start, int target, bool aStar) const {
	DutRoute route;
	const int nodeCount = COLS * ROWS;
	if (start < 0 || start >= nodeCount || target < 0 || target >= nodeCount) {
		return route;
	}

	int startX = getNodeX(start);
	int startY = getNodeY(start);
	int targetX = getNodeX(target);
	int targetY = getNodeY(target);

	if (!isInside(startX, startY) || !isInside(targetX, targetY)) {
		return route;
	}

	Vector<float> gCost(nodeCount, 1e9f);
	Vector<int> parent(nodeCount, -1);
	Vector<bool> visited(nodeCount, false);
	MinHeap<RouteQueueEntry> pq;

	gCost[start] = 0.0f;
	float initialH = aStar ? static_cast<float>(std::abs(startX - targetX) + std::abs(startY - targetY)) : 0.0f;
	pq.push(RouteQueueEntry{initialH, start});

	while (!pq.empty()) {
		RouteQueueEntry topEntry = pq.pop();
		int u = topEntry.vertex;

		if (visited[u]) continue;
		visited[u] = true;
		route.explored.push_back(u);

		if (u == target) break;

		Vector<int> adj = neighbours(u, target);
		for (int i = 0; i < adj.size(); ++i) {
			int v = adj[i];
			if (visited[v]) continue;
			int vx = v % COLS;
			int vy = v / COLS;
			float weight = (cells[v] == DutCellType::MainHighway) ? 0.8f : 1.0f;
			float newG = gCost[u] + weight;
			if (newG < gCost[v]) {
				gCost[v] = newG;
				parent[v] = u;
				float h = aStar ? static_cast<float>(std::abs(vx - targetX) + std::abs(vy - targetY)) : 0.0f;
				pq.push(RouteQueueEntry{newG + h, v});
			}
		}
	}

	if (gCost[target] >= 1e8f) return route;
	route.distance = gCost[target];
	int cur = target;
	int steps = 0;
	while (cur != -1 && steps++ < nodeCount) {
		route.nodes.push_back(cur);
		if (cur == start) break;
		cur = parent[cur];
	}
	route.nodes.reverse();
	return route;
}

DutMapUI::DutMapUI() {
	updateRoute();
}

void DutMapUI::updateRoute() {
	int startX = 12, startY = 3;
	targetPoint.x = clampVal(targetPoint.x, 0, DutMap::COLS - 1);
	targetPoint.y = clampVal(targetPoint.y, 0, DutMap::ROWS - 1);
	int startNode = map.nodeAt(startX, startY);
	int targetNode = map.nodeAt(targetPoint.x, targetPoint.y);
	currentRoute = map.findRoute(startNode, targetNode, useAStar);
	pathProgress = 0.0f;
	shipperProgress = 0.0f;
}

void DutMapUI::openAddBranchModal(const Vector<Branch>& branches) {
	isBranchModalOpen = true;
	branchModalMode = 0;
	editingBranchIdx = -1;
	int nextNum = 1;
	for (int i = 0; i < branches.size(); ++i) {
		String id = branches[i].getId();
		if (id.size() >= 3) {
			int n = id.substr(2).toInt(0);
			nextNum = maxVal(nextNum, n + 1);
		}
	}
	char buf[16];
	std::snprintf(buf, sizeof(buf), "BR%03d", nextNum);
	inputBranchId = buf;
	inputBranchName = "Chi nhánh KTX Bách Khoa";
	inputBranchGridX = 19;
	inputBranchGridY = 5;
	branchModalField = 1;
}

void DutMapUI::openEditBranchModal(int branchIdx, const Vector<Branch>& branches) {
	if (branchIdx < 0 || branchIdx >= branches.size()) return;
	isBranchModalOpen = true;
	branchModalMode = 1;
	editingBranchIdx = branchIdx;
	inputBranchId = branches[branchIdx].getId();
	inputBranchName = branches[branchIdx].getName();
	inputBranchGridX = static_cast<int>(branches[branchIdx].getX());
	inputBranchGridY = static_cast<int>(branches[branchIdx].getY());
	branchModalField = 1;
}

bool DutMapUI::submitBranchModal(Vector<Branch>& branches, RestaurantChain& chain,
	Vector<AccountRecord>& accounts, String& statusMessage, int& statusVersion) {
	if (inputBranchId.empty() || inputBranchName.empty()) {
		statusVersion++;
		statusMessage = "Vui lòng nhập đầy đủ Mã và Tên chi nhánh!";
		return false;
	}

	inputBranchGridX = clampVal(inputBranchGridX, 0, DutMap::COLS - 1);
	inputBranchGridY = clampVal(inputBranchGridY, 0, DutMap::ROWS - 1);

	if (branchModalMode == 0) {
		for (int i = 0; i < branches.size(); ++i) {
			if (branches[i].getId() == inputBranchId) {
				statusVersion++;
				statusMessage = String("Mã chi nhánh ") + inputBranchId + " đã tồn tại!";
				return false;
			}
		}

		Branch newBranch(inputBranchId, inputBranchName, static_cast<float>(inputBranchGridX), static_cast<float>(inputBranchGridY));
		newBranch.setOpeningHours("08:00", "22:30");
		branches.push_back(newBranch);
		chain.addBranch(inputBranchId, inputBranchName, static_cast<float>(inputBranchGridX), static_cast<float>(inputBranchGridY));

		int branchNum = branches.size();
		if (inputBranchId.size() >= 3) {
			branchNum = inputBranchId.substr(2).toInt(branchNum);
		}
		char branchTag[16];
		std::snprintf(branchTag, sizeof(branchTag), "%02d", branchNum);
		String numDigits = branchTag;

		String mgrUser = String("manager") + toString(branchNum);
		String mgrPass = String("mgr") + toString(branchNum) + "pass";
		String mgrSalt = GenerateSalt();
		String mgrHash = HashPassword(mgrPass, mgrSalt);
		String mgrId = String("MGR") + numDigits;
		String mgrName = String("Quản lý ") + inputBranchName;

		accounts.push_back(AccountRecord(mgrId, mgrUser, mgrName, "Manager", inputBranchId, mgrSalt, mgrHash, nullptr));

		for (int i = 1; i <= 3; ++i) {
			char empNumBuf[8];
			std::snprintf(empNumBuf, sizeof(empNumBuf), "%02d", i);
			String empUser = String("employee") + numDigits + empNumBuf;
			String empPass = String("emp") + numDigits + empNumBuf + "pass";
			String empSalt = GenerateSalt();
			String empHash = HashPassword(empPass, empSalt);
			String empId = String("EMP") + numDigits + empNumBuf;
			String empName = String("Nhân viên ") + numDigits + "-" + empNumBuf;

			accounts.push_back(AccountRecord(empId, empUser, empName, "Employee", inputBranchId, empSalt, empHash, nullptr));
		}

		SaveAccountsToFile(accounts);

		statusVersion++;
		statusMessage = String("Đã thêm ") + inputBranchName + " (" + inputBranchId + ")! Tự động tạo 1 Quản lý (" + mgrUser + ") & 3 Nhân viên.";
	} else if (branchModalMode == 1 && editingBranchIdx >= 0 && editingBranchIdx < branches.size()) {
		branches[editingBranchIdx] = Branch(inputBranchId, inputBranchName,
			static_cast<float>(inputBranchGridX), static_cast<float>(inputBranchGridY));
		branches[editingBranchIdx].setOpeningHours("08:00", "22:30");

		chain.clear();
		for (int i = 0; i < branches.size(); ++i) {
			chain.addBranch(branches[i].getId(), branches[i].getName(), branches[i].getX(), branches[i].getY());
		}

		statusVersion++;
		statusMessage = String("Đã cập nhật thông tin chi nhánh: ") + inputBranchName;
	}

	isBranchModalOpen = false;
	return true;
}

void DutMapUI::deleteBranch(int branchIdx, Vector<Branch>& branches, RestaurantChain& chain,
	Vector<AccountRecord>& accounts, String& statusMessage, int& statusVersion) {
	if (branches.size() <= 1) {
		statusVersion++;
		statusMessage = "Không thể xoá chi nhánh cuối cùng của hệ thống!";
		return;
	}
	if (branchIdx < 0 || branchIdx >= branches.size()) return;

	String delId = branches[branchIdx].getId();
	String delName = branches[branchIdx].getName();
	branches.eraseAt(branchIdx);
	chain.removeBranch(delId);

	for (int i = accounts.size() - 1; i >= 0; --i) {
		if (accounts[i].branchId == delId && accounts[i].role != "ChainManager") {
			accounts.eraseAt(i);
		}
	}
	SaveAccountsToFile(accounts);

	startBranchIdx = clampVal(startBranchIdx, 0, branches.size() - 1);
	targetBranchIdx = clampVal(targetBranchIdx, 0, branches.size() - 1);

	if (startBranchIdx == targetBranchIdx && branches.size() > 1) {
		targetBranchIdx = (startBranchIdx + 1) % branches.size();
		targetPoint = {clampVal(static_cast<int>(branches[targetBranchIdx].getX()), 0, DutMap::COLS - 1),
		               clampVal(static_cast<int>(branches[targetBranchIdx].getY()), 0, DutMap::ROWS - 1)};
		isTargetCustom = false;
	}

	if (!branches.empty()) {
		int sx = clampVal(static_cast<int>(branches[startBranchIdx].getX()), 0, DutMap::COLS - 1);
		int sy = clampVal(static_cast<int>(branches[startBranchIdx].getY()), 0, DutMap::ROWS - 1);
		int tx = clampVal(targetPoint.x, 0, DutMap::COLS - 1);
		int ty = clampVal(targetPoint.y, 0, DutMap::ROWS - 1);
		currentRoute = map.findRoute(map.nodeAt(sx, sy), map.nodeAt(tx, ty), useAStar);
		pathProgress = 0.0f;
		shipperProgress = 0.0f;
	}

	statusVersion++;
	statusMessage = String("Đã xoá chi nhánh: ") + delName + " (" + delId + ") và thu hồi các tài khoản liên quan.";
}

void DutMapUI::handleScroll(Vector2 mouse, float wheelMove) {
	if (std::fabs(wheelMove) < 0.01f || isBranchModalOpen) return;
	int step = (wheelMove > 0) ? -1 : 1;
	if (CheckCollisionPointRec(mouse, {980, 540, 275, 120})) {
		branchListScroll = maxVal(0, branchListScroll + step);
	}
}

bool DutMapUI::handleInput(int key, int charPressed, Vector<Branch>& branches, RestaurantChain& chain,
	Vector<AccountRecord>& accounts, Employee* /*currentUser*/, String& statusMessage, int& statusVersion) {
	if (!isBranchModalOpen) return false;

	if (key == KEY_ESCAPE) {
		isBranchModalOpen = false;
		return true;
	}
	if (key == KEY_ENTER) {
		bool ok = submitBranchModal(branches, chain, accounts, statusMessage, statusVersion);
		if (ok && !branches.empty()) {
			startBranchIdx = clampVal(startBranchIdx, 0, branches.size() - 1);
			int sx = clampVal(static_cast<int>(branches[startBranchIdx].getX()), 0, DutMap::COLS - 1);
			int sy = clampVal(static_cast<int>(branches[startBranchIdx].getY()), 0, DutMap::ROWS - 1);
			int tx = clampVal(targetPoint.x, 0, DutMap::COLS - 1);
			int ty = clampVal(targetPoint.y, 0, DutMap::ROWS - 1);
			currentRoute = map.findRoute(map.nodeAt(sx, sy), map.nodeAt(tx, ty), useAStar);
			pathProgress = 0.0f;
			shipperProgress = 0.0f;
		}
		return ok;
	}
	if (key == KEY_TAB) {
		branchModalField = (branchModalField + 1) % 4;
		return true;
	}

	String* targetStr = nullptr;
	if (branchModalField == 0 && branchModalMode == 0) targetStr = &inputBranchId;
	else if (branchModalField == 1) targetStr = &inputBranchName;

	if (targetStr) {
		if (key == KEY_BACKSPACE && !targetStr->empty()) {
			PopUtf8Char(*targetStr);
			return true;
		}
		if (charPressed >= 32) {
			if (branchModalField == 0) {
				if (std::isalnum(charPressed) && targetStr->size() < 12) {
					targetStr->push_back(static_cast<char>(std::toupper(charPressed)));
				}
			} else {
				AppendUtf8Codepoint(*targetStr, charPressed, 40);
			}
			return true;
		}
	} else if (branchModalField == 2 || branchModalField == 3) {
		int& coord = (branchModalField == 2) ? inputBranchGridX : inputBranchGridY;
		if (key == KEY_BACKSPACE) {
			coord /= 10;
			return true;
		}
		if (std::isdigit(charPressed)) {
			coord = coord * 10 + (charPressed - '0');
			coord = clampVal(coord, 0, (branchModalField == 2) ? (DutMap::COLS - 1) : (DutMap::ROWS - 1));
			return true;
		}
	}
	return false;
}

bool DutMapUI::handleClick(Vector2 mouse, Vector<Branch>& branches, RestaurantChain& chain,
	Vector<AccountRecord>& accounts, Employee* currentUser,
	String& statusMessage, int& statusVersion) {
	bool isChainMgr = currentUser && currentUser->canManageAllBranches();

	if (isBranchModalOpen) {
		Rectangle modal = {400, 150, 480, 420};
		if (!CheckCollisionPointRec(mouse, modal)) {
			isBranchModalOpen = false;
			return false;
		}
		if (CheckCollisionPointRec(mouse, {modal.x + 30, modal.y + 90, 420, 36}) && branchModalMode == 0) branchModalField = 0;
		if (CheckCollisionPointRec(mouse, {modal.x + 30, modal.y + 160, 420, 36})) branchModalField = 1;
		if (CheckCollisionPointRec(mouse, {modal.x + 30, modal.y + 230, 200, 36})) branchModalField = 2;
		if (CheckCollisionPointRec(mouse, {modal.x + 250, modal.y + 230, 200, 36})) branchModalField = 3;

		if (CheckCollisionPointRec(mouse, {modal.x + 30, modal.y + 350, 200, 40})) {
			bool ok = submitBranchModal(branches, chain, accounts, statusMessage, statusVersion);
			if (ok && !branches.empty()) {
				startBranchIdx = clampVal(startBranchIdx, 0, branches.size() - 1);
				int sx = clampVal(static_cast<int>(branches[startBranchIdx].getX()), 0, DutMap::COLS - 1);
				int sy = clampVal(static_cast<int>(branches[startBranchIdx].getY()), 0, DutMap::ROWS - 1);
				int tx = clampVal(targetPoint.x, 0, DutMap::COLS - 1);
				int ty = clampVal(targetPoint.y, 0, DutMap::ROWS - 1);
				currentRoute = map.findRoute(map.nodeAt(sx, sy), map.nodeAt(tx, ty), useAStar);
				pathProgress = 0.0f;
				shipperProgress = 0.0f;
			}
			return ok;
		}
		if (CheckCollisionPointRec(mouse, {modal.x + 250, modal.y + 350, 200, 40})) {
			isBranchModalOpen = false;
			return false;
		}
		return false;
	}

	Rectangle mapRec = {mapOriginX, mapOriginY, DutMap::COLS * mapCellSize, DutMap::ROWS * mapCellSize};
	if (CheckCollisionPointRec(mouse, mapRec)) {
		int cellX = static_cast<int>((mouse.x - mapOriginX) / mapCellSize);
		int cellY = static_cast<int>((mouse.y - mapOriginY) / mapCellSize);
		cellX = clampVal(cellX, 0, DutMap::COLS - 1);
		cellY = clampVal(cellY, 0, DutMap::ROWS - 1);

		int clickedBranch = -1;
		for (int i = 0; i < branches.size(); ++i) {
			int bx = clampVal(static_cast<int>(branches[i].getX()), 0, DutMap::COLS - 1);
			int by = clampVal(static_cast<int>(branches[i].getY()), 0, DutMap::ROWS - 1);
			float px = mapOriginX + bx * mapCellSize + mapCellSize * 0.5f;
			float py = mapOriginY + by * mapCellSize + mapCellSize * 0.5f;
			float dx = mouse.x - px;
			float dy = mouse.y - py;
			if (dx * dx + dy * dy <= 450.0f) {
				clickedBranch = i;
				break;
			}
		}

		if (selectionStep == 0) {
			if (clickedBranch >= 0) {
				startBranchIdx = clickedBranch;
				selectionStep = 1;
				statusVersion++;
				statusMessage = String("Đã chọn điểm xuất phát: ") + branches[startBranchIdx].getName() + ". Hãy chọn điểm đến!";
			} else {
				statusVersion++;
				statusMessage = "Vui lòng chọn 1 chi nhánh làm điểm xuất phát!";
				return false;
			}
		} else {
			if (clickedBranch >= 0) {
				if (clickedBranch == startBranchIdx) {
					statusVersion++;
					statusMessage = "Điểm đến trùng với điểm xuất phát. Vui lòng chọn điểm khác!";
					return false;
				}
				targetBranchIdx = clickedBranch;
				isTargetCustom = false;
				int tbx = clampVal(static_cast<int>(branches[targetBranchIdx].getX()), 0, DutMap::COLS - 1);
				int tby = clampVal(static_cast<int>(branches[targetBranchIdx].getY()), 0, DutMap::ROWS - 1);
				targetPoint = {tbx, tby};
				statusVersion++;
				statusMessage = String("Đã chọn điểm đến: ") + branches[targetBranchIdx].getName();
			} else {
				isTargetCustom = true;
				targetBranchIdx = -1;
				targetPoint = {cellX, cellY};
				statusVersion++;
				statusMessage = String("Đã chọn địa chỉ giao hàng tại toạ độ (") + toString(cellX) + ", " + toString(cellY) + ")";
			}
		}

		if (!branches.empty()) {
			startBranchIdx = clampVal(startBranchIdx, 0, branches.size() - 1);
			int sx = clampVal(static_cast<int>(branches[startBranchIdx].getX()), 0, DutMap::COLS - 1);
			int sy = clampVal(static_cast<int>(branches[startBranchIdx].getY()), 0, DutMap::ROWS - 1);
			int startNode = map.nodeAt(sx, sy);
			int targetNode = map.nodeAt(targetPoint.x, targetPoint.y);
			currentRoute = map.findRoute(startNode, targetNode, useAStar);
			pathProgress = 0.0f;
			shipperProgress = 0.0f;
		}
		return false;
	}

	const float px = 995.0f;
	const float pw = 245.0f;

	Rectangle resetStartBtn = {px, 312, pw, 32};
	if (CheckCollisionPointRec(mouse, resetStartBtn)) {
		selectionStep = 0;
		statusVersion++;
		statusMessage = "Hãy nhấp vào 1 chi nhánh trên bản đồ để chọn làm điểm xuất phát mới!";
		return false;
	}

	Rectangle algoBtn = {px, 352, pw, 32};
	if (CheckCollisionPointRec(mouse, algoBtn)) {
		useAStar = !useAStar;
		pathProgress = 0.0f;
		shipperProgress = 0.0f;
		if (!branches.empty()) {
			startBranchIdx = clampVal(startBranchIdx, 0, branches.size() - 1);
			int sx = clampVal(static_cast<int>(branches[startBranchIdx].getX()), 0, DutMap::COLS - 1);
			int sy = clampVal(static_cast<int>(branches[startBranchIdx].getY()), 0, DutMap::ROWS - 1);
			int startNode = map.nodeAt(sx, sy);
			int targetNode = map.nodeAt(targetPoint.x, targetPoint.y);
			currentRoute = map.findRoute(startNode, targetNode, useAStar);
		}
		statusVersion++;
		statusMessage = useAStar ? "Đã chuyển sang thuật toán A* (Tối ưu)" : "Đã chuyển sang thuật toán Dijkstra";
		return false;
	}

	if (isChainMgr) {
		Rectangle addBrBtn = {px, 478, pw, 28};
		if (CheckCollisionPointRec(mouse, addBrBtn)) {
			openAddBranchModal(branches);
			return false;
		}

		const float branchRowStartY = 514.0f;
		const float branchRowHeight = 26.0f;
		for (int i = 0; i < branches.size(); ++i) {
			float rowY = branchRowStartY + i * branchRowHeight;
			if (rowY > 690) break;
			Rectangle editBtn = {px + pw - 74, rowY, 34, 20};
			Rectangle delBtn = {px + pw - 36, rowY, 34, 20};
			if (CheckCollisionPointRec(mouse, editBtn)) {
				openEditBranchModal(i, branches);
				return false;
			}
			if (CheckCollisionPointRec(mouse, delBtn)) {
				deleteBranch(i, branches, chain, accounts, statusMessage, statusVersion);
				return true;
			}
		}
	}
	return false;
}

void DutMapUI::draw(Vector2 mouse, const Vector<Branch>& branches, Employee* currentUser) {
	bool isChainMgr = currentUser && currentUser->canManageAllBranches();
	float dt = Anim::Dt();
	pathProgress += dt * 140.0f;

	int totalExplored = currentRoute.explored.size();
	int expCount = minVal(totalExplored, static_cast<int>(pathProgress));
	bool searchFinished = (pathProgress >= static_cast<float>(totalExplored));

	DrawUiText("Bản Đồ Khu Vực ĐH Bách Khoa & Tìm Đường", 245, 104, 24, Color{35, 55, 53, 255});
	DrawUiTextFitted("Khu vực: ĐH Bách Khoa - Chợ Hoà Khánh - Ngô Sĩ Liên - Âu Cơ  •  Nhấp chọn 2 điểm để tìm lộ trình tối ưu",
		245, 136, 1000, 13, Color{104, 115, 108, 255});

	float mapW = DutMap::COLS * mapCellSize;
	float mapH = DutMap::ROWS * mapCellSize;
	DrawRectangle(static_cast<int>(mapOriginX), static_cast<int>(mapOriginY), static_cast<int>(mapW), static_cast<int>(mapH), Color{236, 233, 222, 255});

	for (int y = 0; y < DutMap::ROWS; ++y) {
		for (int x = 0; x < DutMap::COLS; ++x) {
			Rectangle cell = {mapOriginX + x * mapCellSize, mapOriginY + y * mapCellSize, mapCellSize, mapCellSize};
			DutCellType type = map.getCellType(x, y);

			switch (type) {
			case DutCellType::MainHighway:
				DrawRectangleRec(cell, Color{55, 62, 66, 255});
				if (x == 11 && (y % 2 == 0)) {
					DrawRectangle(static_cast<int>(cell.x + cell.width - 1), static_cast<int>(cell.y + 4), 2, 16, Color{250, 218, 94, 230});
				}
				break;
			case DutCellType::Road:
				DrawRectangleRec(cell, Color{75, 82, 86, 255});
				break;
			case DutCellType::Campus:
				DrawRectangleRec(cell, Color{216, 236, 214, 255});
				DrawCircle(static_cast<int>(cell.x + 6), static_cast<int>(cell.y + 6), 3, Color{145, 195, 140, 180});
				break;
			case DutCellType::CampusBuilding:
				DrawRectangleRounded(cell, 0.15f, 4, Color{180, 75, 65, 255});
				DrawRectangleLines(static_cast<int>(cell.x), static_cast<int>(cell.y), static_cast<int>(cell.width), static_cast<int>(cell.height), Color{140, 50, 40, 255});
				break;
			case DutCellType::Market:
				DrawRectangleRec(cell, Color{242, 185, 130, 255});
				break;
			case DutCellType::Water:
				DrawRectangleRec(cell, Color{90, 155, 205, 255});
				if ((x + y) % 2 == 0) {
					DrawCircle(static_cast<int>(cell.x + 12), static_cast<int>(cell.y + 12), 4, Color{125, 185, 225, 180});
				}
				break;
			case DutCellType::Park:
				DrawRectangleRec(cell, Color{195, 225, 190, 255});
				DrawCircle(static_cast<int>(cell.x + 12), static_cast<int>(cell.y + 12), 5, Color{120, 180, 115, 200});
				break;
			case DutCellType::Building:
			default:
				DrawRectangleRec(cell, Color{232, 228, 218, 255});
				DrawRectangleLines(static_cast<int>(cell.x), static_cast<int>(cell.y), static_cast<int>(cell.width), static_cast<int>(cell.height), Color{218, 214, 204, 255});
				break;
			}
		}
	}

	DrawUiText("TRƯỜNG ĐẠI HỌC BÁCH KHOA (DUT)", mapOriginX + 13 * mapCellSize + 4, mapOriginY + 0.5f * mapCellSize, 11, Color{30, 85, 55, 230});
	DrawUiText("CHỢ HOÀ KHÁNH", mapOriginX + 7 * mapCellSize + 4, mapOriginY + 11.2f * mapCellSize, 10, Color{130, 65, 30, 240});
	DrawUiText("HỒ BÀU TRÀM", mapOriginX + 26 * mapCellSize + 4, mapOriginY + 15.0f * mapCellSize, 10, Color{45, 95, 140, 240});

	DrawUiText("<- ÂU CƠ (BÀ NÀ)", mapOriginX + 1 * mapCellSize, mapOriginY + 13.2f * mapCellSize, 9, Color{245, 245, 245, 220});
	DrawUiText("NGÔ SĨ LIÊN (KTX) ->", mapOriginX + 20 * mapCellSize, mapOriginY + 5.2f * mapCellSize, 9, Color{245, 245, 245, 220});
	DrawUiText("QL1A (NGUYỄN LƯƠNG BẰNG)", mapOriginX + 11.1f * mapCellSize, mapOriginY + 19.1f * mapCellSize, 9, Color{250, 225, 110, 240});

	for (int i = 0; i < expCount; ++i) {
		int u = currentRoute.explored[i];
		int ux = map.getNodeX(u);
		int uy = map.getNodeY(u);
		Rectangle r = {mapOriginX + ux * mapCellSize + 7, mapOriginY + uy * mapCellSize + 7, 10, 10};
		Color expCol = useAStar ? Color{165, 215, 195, 150} : Color{180, 200, 225, 150};
		DrawRectangleRounded(r, 0.4f, 4, expCol);
	}

	if (searchFinished && currentRoute.nodes.size() > 1) {
		float routeTime = (pathProgress - static_cast<float>(totalExplored));
		int maxSegments = currentRoute.nodes.size() - 1;
		int visibleSegments = clampVal(static_cast<int>(routeTime * 4.0f), 0, maxSegments);

		for (int i = 0; i < visibleSegments; ++i) {
			int u = currentRoute.nodes[i];
			int v = currentRoute.nodes[i + 1];
			Vector2 p1 = {mapOriginX + map.getNodeX(u) * mapCellSize + mapCellSize * 0.5f,
				          mapOriginY + map.getNodeY(u) * mapCellSize + mapCellSize * 0.5f};
			Vector2 p2 = {mapOriginX + map.getNodeX(v) * mapCellSize + mapCellSize * 0.5f,
				          mapOriginY + map.getNodeY(v) * mapCellSize + mapCellSize * 0.5f};

			DrawLineEx(p1, p2, 6.0f, useAStar ? Color{245, 130, 45, 230} : Color{35, 155, 95, 230});
			DrawCircleV(p1, 4.0f, useAStar ? Color{255, 175, 80, 255} : Color{55, 185, 125, 255});
			if (i == visibleSegments - 1) {
				DrawCircleV(p2, 4.0f, useAStar ? Color{255, 175, 80, 255} : Color{55, 185, 125, 255});
			}
		}

		if (visibleSegments >= maxSegments) {
			shipperProgress += dt * 6.0f;
			if (shipperProgress >= static_cast<float>(maxSegments)) {
				shipperProgress = 0.0f;
			}

			int segIdx = clampVal(static_cast<int>(shipperProgress), 0, maxSegments - 1);
			float segFrac = shipperProgress - static_cast<float>(segIdx);
			int u = currentRoute.nodes[segIdx];
			int v = currentRoute.nodes[segIdx + 1];
			Vector2 p1 = {mapOriginX + map.getNodeX(u) * mapCellSize + mapCellSize * 0.5f,
				          mapOriginY + map.getNodeY(u) * mapCellSize + mapCellSize * 0.5f};
			Vector2 p2 = {mapOriginX + map.getNodeX(v) * mapCellSize + mapCellSize * 0.5f,
				          mapOriginY + map.getNodeY(v) * mapCellSize + mapCellSize * 0.5f};
			Vector2 curPos = {p1.x + (p2.x - p1.x) * segFrac, p1.y + (p2.y - p1.y) * segFrac};

			float pulse = 0.5f + 0.5f * std::sin(GetTime() * 8.0f);
			DrawCircleV(curPos, 12.0f + 3.0f * pulse, Color{255, 140, 50, static_cast<unsigned char>(100.0f * (1.0f - pulse))});
			DrawCircleV(curPos, 9.0f, Color{235, 95, 30, 255});
			DrawCircleV(curPos, 6.0f, RAYWHITE);
			DrawCircleV(curPos, 3.5f, Color{35, 55, 53, 255});
		}
	}

	for (int i = 0; i < branches.size(); ++i) {
		int bx = static_cast<int>(branches[i].getX());
		int by = static_cast<int>(branches[i].getY());
		float px = mapOriginX + bx * mapCellSize + mapCellSize * 0.5f;
		float py = mapOriginY + by * mapCellSize + mapCellSize * 0.5f;

		bool isStart = (i == startBranchIdx);
		bool isDest = (!isTargetCustom && i == targetBranchIdx);

		Color pinCol = isStart ? Color{35, 145, 85, 255} : (isDest ? Color{215, 85, 50, 255} : Color{35, 55, 53, 255});
		float radius = (isStart || isDest) ? 12.0f : 9.0f;

		DrawCircle(static_cast<int>(px), static_cast<int>(py), radius + 2.0f, Color{255, 255, 255, 200});
		DrawCircle(static_cast<int>(px), static_cast<int>(py), radius, pinCol);
		DrawCircle(static_cast<int>(px), static_cast<int>(py), 4.0f, RAYWHITE);

		String tag = branches[i].getName();
		if (isStart) tag = String("[XUẤT PHÁT] ") + tag;
		else if (isDest) tag = String("[ĐÍCH ĐẾN] ") + tag;

		float tagW = MeasureUiText(tag.c_str(), 11) + 12.0f;
		float tagX = clampVal(px - tagW * 0.5f, mapOriginX + 4.0f, mapOriginX + mapW - tagW - 4.0f);
		float tagY = clampVal(py + 12.0f, mapOriginY + 4.0f, mapOriginY + mapH - 22.0f);
		Rectangle tagRec = {tagX, tagY, tagW, 18};
		DrawRectangleRounded(tagRec, 0.3f, 4, (isStart || isDest) ? pinCol : Color{255, 255, 255, 235});
		DrawUiTextInRect(tag.c_str(), tagRec, 11, (isStart || isDest) ? RAYWHITE : Color{35, 55, 53, 255}, 0, 4.0f);
	}

	if (isTargetCustom) {
		float tx = mapOriginX + targetPoint.x * mapCellSize + mapCellSize * 0.5f;
		float ty = mapOriginY + targetPoint.y * mapCellSize + mapCellSize * 0.5f;
		DrawCircle(static_cast<int>(tx), static_cast<int>(ty), 10.0f, Color{225, 75, 45, 255});
		DrawCircle(static_cast<int>(tx), static_cast<int>(ty), 4.0f, RAYWHITE);

		const char* destTag = "[GIAO TẬN NƠI]";
		float dtw = MeasureUiText(destTag, 11) + 12.0f;
		Rectangle drec = {tx - dtw * 0.5f, ty + 12.0f, dtw, 18};
		DrawRectangleRounded(drec, 0.3f, 4, Color{225, 75, 45, 255});
		DrawUiTextInRect(destTag, drec, 11, RAYWHITE, 0, 4.0f);
	}

	Rectangle rightPanel = {980, 168, 275, 540};
	DrawRectangleRounded(rightPanel, 0.05f, 4, RAYWHITE);
	DrawRectangleRoundedLines(rightPanel, 0.05f, 4, Color{220, 228, 221, 255});

	const float rpx = 995.0f;
	const float rpw = 245.0f;

	DrawUiTextFitted(useAStar ? "THUẬT TOÁN A* (TỐI ƯU)" : "THUẬT TOÁN DIJKSTRA", rpx, 186, rpw, 13,
		useAStar ? Color{215, 105, 30, 255} : Color{35, 125, 75, 255});

	String startName = (startBranchIdx >= 0 && startBranchIdx < branches.size())
		? branches[startBranchIdx].getName() : String("Chưa chọn");
	String targetName = isTargetCustom
		? (String("Toạ độ (") + toString(targetPoint.x) + ", " + toString(targetPoint.y) + ")")
		: ((targetBranchIdx >= 0 && targetBranchIdx < branches.size())
			? branches[targetBranchIdx].getName() : String("Chưa chọn"));

	DrawUiTextFitted((String("1. Xuất phát: ") + startName).c_str(), rpx, 208, rpw, 13, Color{35, 135, 85, 255});
	DrawUiTextFitted((String("2. Điểm đến: ") + targetName).c_str(), rpx, 230, rpw, 13, Color{215, 85, 45, 255});

	if (currentRoute.nodes.size() > 1) {
		float distanceKm = currentRoute.distance * 0.12f;
		int estMinutes = maxVal(2, static_cast<int>(distanceKm * 3.5f + 2.0f));
		char statBuf[100];
		std::snprintf(statBuf, sizeof(statBuf), "Quãng đường: %.1f km (%d ô)  |  Dự kiến: %d phút",
			distanceKm, currentRoute.nodes.size(), estMinutes);
		DrawUiTextFitted(statBuf, rpx, 256, rpw, 12, Color{54, 66, 58, 255});
	} else {
		DrawUiTextFitted("Quãng đường: -- km  |  Dự kiến: --", rpx, 256, rpw, 12, Color{120, 130, 125, 255});
	}

	Rectangle badgeRec = {rpx, 280, rpw, 22};
	if (!searchFinished) {
		DrawRectangleRounded(badgeRec, 0.3f, 4, Color{255, 243, 220, 255});
		char sBuf[64];
		std::snprintf(sBuf, sizeof(sBuf), "⚡ Đang duyệt tìm đường... (%d/%d ô)", expCount, totalExplored);
		DrawUiTextInRect(sBuf, badgeRec, 11, Color{190, 110, 30, 255}, 0, 4.0f);
	} else if (currentRoute.nodes.size() <= 1) {
		DrawRectangleRounded(badgeRec, 0.3f, 4, Color{255, 230, 230, 255});
		DrawUiTextInRect("✗ Không tìm thấy đường đi tới vị trí này", badgeRec, 11, Color{200, 50, 40, 255}, 0, 4.0f);
	} else {
		DrawRectangleRounded(badgeRec, 0.3f, 4, Color{232, 246, 236, 255});
		char sBuf[64];
		std::snprintf(sBuf, sizeof(sBuf), "✓ Đã tìm lộ trình tối ưu (Duyệt %d ô)", totalExplored);
		DrawUiTextInRect(sBuf, badgeRec, 11, Color{35, 135, 85, 255}, 0, 4.0f);
	}

	Rectangle resetStartBtn = {rpx, 312, rpw, 32};
	Anim::Button(resetStartBtn, Color{235, 240, 235, 255}, mouse, true, 0.2f);
	DrawUiTextInRect(selectionStep == 0 ? "-> Hãy click CN xuất phát..." : "Đổi điểm xuất phát",
		resetStartBtn, 12, Color{35, 105, 78, 255}, 0, 4.0f);

	Rectangle algoBtn = {rpx, 352, rpw, 32};
	Anim::Button(algoBtn, Color{224, 231, 220, 255}, mouse, true, 0.2f);
	DrawUiTextInRect(useAStar ? "Đổi sang: Thuật toán Dijkstra" : "Đổi sang: Thuật toán A*",
		algoBtn, 12, Color{54, 66, 58, 255}, 0, 6.0f);

	Rectangle infoCard = {rpx, 392, rpw, 46};
	DrawRectangleRounded(infoCard, 0.15f, 4, Color{242, 245, 241, 255});
	DrawRectangleRoundedLines(infoCard, 0.15f, 4, Color{215, 224, 218, 255});
	if (useAStar) {
		DrawUiTextFitted("Thuật toán A* (Heuristic + Min-Heap)", rpx + 8, 396, rpw - 16, 11, Color{215, 105, 30, 255});
		DrawUiTextFitted("Kết hợp chi phí thực tế & khoảng cách Manhattan", rpx + 8, 410, rpw - 16, 10, Color{85, 102, 92, 255});
		DrawUiTextFitted("định hướng trực tiếp tới đích, duyệt ít ô hơn.", rpx + 8, 423, rpw - 16, 10, Color{85, 102, 92, 255});
	} else {
		DrawUiTextFitted("Thuật toán Dijkstra (Quét lan toả + Min-Heap)", rpx + 8, 396, rpw - 16, 11, Color{35, 125, 75, 255});
		DrawUiTextFitted("Duyệt đều theo hình gợn sóng mở rộng mọi hướng,", rpx + 8, 410, rpw - 16, 10, Color{85, 102, 92, 255});
		DrawUiTextFitted("đảm bảo tìm thấy đường ngắn nhất tuyệt đối.", rpx + 8, 423, rpw - 16, 10, Color{85, 102, 92, 255});
	}

	DrawLine(static_cast<int>(rpx), 448, static_cast<int>(rpx + rpw), 448, Color{225, 232, 226, 255});

	if (isChainMgr) {
		DrawUiText("QUẢN LÝ CHI NHÁNH (GĐ CHUỖI)", rpx, 458, 12, Color{35, 55, 53, 255});

		Rectangle addBrBtn = {rpx, 478, rpw, 28};
		Anim::Button(addBrBtn, Color{35, 105, 78, 255}, mouse, true, 0.2f);
		DrawUiTextInRect("+ THÊM CHI NHÁNH MỚI", addBrBtn, 11, RAYWHITE, 0, 4.0f);

		const float branchRowStartY = 514.0f;
		const float branchRowHeight = 26.0f;
		for (int i = 0; i < branches.size(); ++i) {
			float rowY = branchRowStartY + i * branchRowHeight;
			if (rowY > 690) break;
			String binfo = branches[i].getId() + " - " + branches[i].getName();
			DrawUiTextFitted(binfo.c_str(), rpx, rowY + 3, rpw - 78, 11, Color{45, 56, 50, 255});

			Rectangle editBtn = {rpx + rpw - 74, rowY, 34, 20};
			Rectangle delBtn = {rpx + rpw - 36, rowY, 34, 20};
			Anim::Button(editBtn, Color{180, 110, 40, 255}, mouse, true, 0.2f);
			DrawUiTextInRect("Sửa", editBtn, 10, RAYWHITE, 0, 2.0f);
			Anim::Button(delBtn, Color{195, 65, 50, 255}, mouse, branches.size() > 1, 0.2f);
			DrawUiTextInRect("Xoá", delBtn, 10, RAYWHITE, 0, 2.0f);
		}
	} else {
		DrawUiText("DANH SÁCH CHI NHÁNH", rpx, 458, 12, Color{35, 55, 53, 255});
		const float branchRowStartY = 484.0f;
		const float branchRowHeight = 26.0f;
		for (int i = 0; i < branches.size(); ++i) {
			float rowY = branchRowStartY + i * branchRowHeight;
			if (rowY > 690) break;
			String binfo = branches[i].getId() + " - " + branches[i].getName();
			DrawUiTextFitted(binfo.c_str(), rpx, rowY + 3, rpw, 11, Color{45, 56, 50, 255});
		}
	}

	if (isBranchModalOpen) {
		DrawRectangle(0, 0, 1280, 760, Color{0, 0, 0, 140});
		Rectangle modal = {400, 150, 480, 420};
		DrawRectangleRounded(modal, 0.05f, 8, RAYWHITE);
		DrawRectangleRoundedLines(modal, 0.05f, 8, Color{35, 105, 78, 255});

		const char* title = branchModalMode == 0 ? "THÊM CHI NHÁNH MỚI" : "CHỈNH SỬA CHI NHÁNH";
		DrawUiTextFitted(title, modal.x + 30, modal.y + 24, 420, 18, Color{35, 55, 53, 255});
		DrawUiTextFitted("Tự động sinh 1 tài khoản Quản lý và 3 Nhân viên khi thêm", modal.x + 30, modal.y + 48, 420, 12, Color{104, 115, 108, 255});

		DrawUiText("Mã chi nhánh (VD: BR005):", modal.x + 30, modal.y + 70, 12, Color{35, 55, 53, 255});
		Rectangle idBox = {modal.x + 30, modal.y + 90, 420, 36};
		DrawUiInputBox(idBox, inputBranchId, "BR005", branchModalField == 0, mouse);

		DrawUiText("Tên chi nhánh:", modal.x + 30, modal.y + 138, 12, Color{35, 55, 53, 255});
		Rectangle nameBox = {modal.x + 30, modal.y + 158, 420, 36};
		DrawUiInputBox(nameBox, inputBranchName, "Nhập tên chi nhánh...", branchModalField == 1, mouse);

		DrawUiText("Toạ độ X trên bản đồ (0..29):", modal.x + 30, modal.y + 208, 12, Color{35, 55, 53, 255});
		Rectangle xBox = {modal.x + 30, modal.y + 228, 200, 36};
		DrawUiInputBox(xBox, toString(inputBranchGridX), "12", branchModalField == 2, mouse);

		DrawUiText("Toạ độ Y trên bản đồ (0..19):", modal.x + 250, modal.y + 208, 12, Color{35, 55, 53, 255});
		Rectangle yBox = {modal.x + 250, modal.y + 228, 200, 36};
		DrawUiInputBox(yBox, toString(inputBranchGridY), "3", branchModalField == 3, mouse);

		DrawUiTextFitted("Nhấn Tab để chuyển ô  •  Enter để Lưu  •  Esc để Huỷ", modal.x + 30, modal.y + 280, 420, 12, Color{104, 115, 108, 255});

		Rectangle saveBtn = {modal.x + 30, modal.y + 340, 200, 42};
		Rectangle cancelBtn = {modal.x + 250, modal.y + 340, 200, 42};
		Anim::Button(saveBtn, Color{35, 105, 78, 255}, mouse, true, 0.2f);
		DrawUiTextInRect("LƯU CHI NHÁNH", saveBtn, 13, RAYWHITE, 0, 8.0f);
		Anim::Button(cancelBtn, Color{150, 160, 152, 255}, mouse, true, 0.2f);
		DrawUiTextInRect("HUỶ BỎ", cancelBtn, 13, RAYWHITE, 0, 8.0f);
	}
}
