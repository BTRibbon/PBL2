#include "DutMapUI.h"
#include "UIFont.h"
#include "UIAnim.h"
#include "../models/AccountRecord.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

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

	// 1. Trục chính: Đại lộ Nguyễn Lương Bằng (QL1A) - 2 làn xe, chạy dọc từ Bắc xuống Nam
	for (int y = 0; y < ROWS; ++y) {
		setCell(11, y, DutCellType::MainHighway, true);
		setCell(12, y, DutCellType::MainHighway, true);
	}

	// 2. Đường Ngô Sĩ Liên: từ Nguyễn Lương Bằng đi về phía Đông (sang KTX và khu dân cư)
	for (int x = 11; x < COLS; ++x) {
		setCell(x, 5, DutCellType::Road, true);
	}

	// 3. Đường Âu Cơ (đi KCN Hoà Khánh / Bà Nà): từ Chợ Hoà Khánh đi về phía Tây
	for (int x = 0; x <= 12; ++x) {
		setCell(x, 13, DutCellType::Road, true);
	}

	// 4. Đường Lạc Long Quân: chạy song song phía Tây
	for (int y = 0; y <= 14; ++y) {
		setCell(4, y, DutCellType::Road, true);
	}

	// 5. Đường Đồng Kè: nối Lạc Long Quân và Nguyễn Lương Bằng
	for (int x = 4; x <= 12; ++x) {
		setCell(x, 9, DutCellType::Road, true);
	}

	// 6. Đường Nam Cao / Trần Văn Kỷ (phía Nam)
	for (int x = 11; x <= 25; ++x) {
		setCell(x, 18, DutCellType::Road, true);
	}

	// 7. Đường Mộc Bài: trục Bắc - Nam nối Ngô Sĩ Liên và Nam Cao
	for (int y = 5; y <= 18; ++y) {
		setCell(19, y, DutCellType::Road, true);
	}

	// 8. Đường Kiệt số 6 (Bắc) và Đường Đoàn Phú Tứ / Ninh Tốn (Nam)
	for (int x = 4; x <= 11; ++x) {
		setCell(x, 2, DutCellType::Road, true);
		setCell(x, 16, DutCellType::Road, true);
	}

	// 9. Lối đi nội bộ & Cổng chính Trường ĐH Bách Khoa (54 Nguyễn Lương Bằng)
	for (int x = 12; x <= 24; ++x) {
		setCell(x, 3, DutCellType::Road, true); // Trục cổng chính vào DUT
		setCell(x, 1, DutCellType::Road, true); // Đường vành đai phía Bắc DUT
	}
	for (int y = 1; y <= 5; ++y) {
		setCell(16, y, DutCellType::Road, true); // Trục giữa giảng đường Bách Khoa
		setCell(22, y, DutCellType::Road, true); // Lối sang sân bóng & KTX
	}

	// 10. Đường ven hồ Bàu Tràm
	for (int y = 14; y <= 18; ++y) {
		setCell(24, y, DutCellType::Road, true);
	}

	// 11. Khuôn viên Trường Đại học Bách Khoa - ĐH Đà Nẵng (DUT)
	setRect(13, 0, 25, 4, DutCellType::Campus, false);
	// Mở lại các đường nội bộ DUT
	for (int x = 12; x <= 24; ++x) { setCell(x, 3, DutCellType::Road, true); setCell(x, 1, DutCellType::Road, true); }
	for (int y = 1; y <= 5; ++y) { setCell(16, y, DutCellType::Road, true); setCell(22, y, DutCellType::Road, true); }
	setCell(14, 2, DutCellType::CampusBuilding, false); // Khu A Hiệu bộ
	setCell(15, 2, DutCellType::CampusBuilding, false);
	setCell(18, 2, DutCellType::CampusBuilding, false); // Khu B Giảng đường
	setCell(19, 2, DutCellType::CampusBuilding, false);
	setCell(23, 2, DutCellType::CampusBuilding, false); // Sân vận động Bách Khoa
	setCell(24, 2, DutCellType::CampusBuilding, false);

	// 12. Chợ Hoà Khánh
	setRect(7, 11, 10, 12, DutCellType::Market, false);
	// Lối đi vào chợ
	setCell(9, 12, DutCellType::Road, true);

	// 13. Hồ Bàu Tràm (Góc Đông Nam)
	setRect(26, 14, 29, 19, DutCellType::Water, false);
	setRect(25, 16, 25, 17, DutCellType::Water, false);

	// 14. Công viên & Cây xanh
	setRect(5, 4, 8, 7, DutCellType::Park, false);
	setRect(14, 7, 17, 8, DutCellType::Park, false);
}

std::vector<int> DutMap::neighbours(int node, int target) const {
	if (node < 0 || node >= COLS * ROWS) return {};
	int x = node % COLS;
	int y = node / COLS;
	std::vector<int> result;
	const std::array<MapGridPoint, 4> dirs{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
	for (const auto& d : dirs) {
		int nx = x + d.x;
		int ny = y + d.y;
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

	std::vector<float> gCost(nodeCount, 1e9f);
	std::vector<int> parent(nodeCount, -1);
	std::vector<bool> visited(nodeCount, false);
	using P = std::pair<float, int>;
	std::priority_queue<P, std::vector<P>, std::greater<P>> pq;

	gCost[start] = 0.0f;
	float initialH = aStar ? static_cast<float>(std::abs(startX - targetX) + std::abs(startY - targetY)) : 0.0f;
	pq.push({initialH, start});

	while (!pq.empty()) {
		auto [fCost, u] = pq.top();
		pq.pop();

		if (visited[u]) continue;
		visited[u] = true;
		route.explored.push_back(u);

		if (u == target) break;

		for (int v : neighbours(u, target)) {
			if (visited[v]) continue;
			int vx = v % COLS;
			int vy = v / COLS;
			// MainHighway (QL1A) has lower cost 0.8f (faster), normal road 1.0f
			float weight = (cells[v] == DutCellType::MainHighway) ? 0.8f : 1.0f;
			float newG = gCost[u] + weight;
			if (newG < gCost[v]) {
				gCost[v] = newG;
				parent[v] = u;
				float h = aStar ? static_cast<float>(std::abs(vx - targetX) + std::abs(vy - targetY)) : 0.0f;
				pq.push({newG + h, v});
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
	std::reverse(route.nodes.begin(), route.nodes.end());
	return route;
}

DutMapUI::DutMapUI() {
	updateRoute();
}

void DutMapUI::updateRoute() {
	int startX = 12, startY = 3;
	targetPoint.x = std::clamp(targetPoint.x, 0, DutMap::COLS - 1);
	targetPoint.y = std::clamp(targetPoint.y, 0, DutMap::ROWS - 1);
	int startNode = map.nodeAt(startX, startY);
	int targetNode = map.nodeAt(targetPoint.x, targetPoint.y);
	currentRoute = map.findRoute(startNode, targetNode, useAStar);
	pathProgress = 0.0f;
	shipperProgress = 0.0f;
}

void DutMapUI::openAddBranchModal(const std::vector<Branch>& branches) {
	isBranchModalOpen = true;
	branchModalMode = 0;
	editingBranchIdx = -1;
	int nextNum = 1;
	for (const auto& b : branches) {
		std::string id = b.getId();
		if (id.size() >= 3) {
			try {
				int n = std::stoi(id.substr(2));
				nextNum = std::max(nextNum, n + 1);
			} catch (...) {}
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

void DutMapUI::openEditBranchModal(int branchIdx, const std::vector<Branch>& branches) {
	if (branchIdx < 0 || branchIdx >= static_cast<int>(branches.size())) return;
	isBranchModalOpen = true;
	branchModalMode = 1;
	editingBranchIdx = branchIdx;
	inputBranchId = branches[branchIdx].getId();
	inputBranchName = branches[branchIdx].getName();
	inputBranchGridX = static_cast<int>(branches[branchIdx].getX());
	inputBranchGridY = static_cast<int>(branches[branchIdx].getY());
	branchModalField = 1;
}

bool DutMapUI::submitBranchModal(std::vector<Branch>& branches,
	std::vector<AccountRecord>& accounts, std::vector<std::unique_ptr<User>>& userPool,
	std::string& statusMessage, int& statusVersion) {
	if (inputBranchId.empty() || inputBranchName.empty()) {
		statusVersion++;
		statusMessage = "Vui lòng nhập đầy đủ Mã và Tên chi nhánh!";
		return false;
	}

	inputBranchGridX = std::clamp(inputBranchGridX, 0, DutMap::COLS - 1);
	inputBranchGridY = std::clamp(inputBranchGridY, 0, DutMap::ROWS - 1);

	if (branchModalMode == 0) {
		// Check duplicate ID
		for (const auto& b : branches) {
			if (b.getId() == inputBranchId) {
				statusVersion++;
				statusMessage = "Mã chi nhánh " + inputBranchId + " đã tồn tại!";
				return false;
			}
		}

		// Add new branch
		Branch newBranch(inputBranchId, inputBranchName, static_cast<float>(inputBranchGridX), static_cast<float>(inputBranchGridY));
		newBranch.setOpeningHours("08:00", "22:30");
		branches.push_back(newBranch);

		// Extract numeric part for account generation (e.g., BR005 -> 5, "05")
		int branchNum = static_cast<int>(branches.size());
		std::string numDigits = "01";
		if (inputBranchId.size() >= 3) {
			try {
				branchNum = std::stoi(inputBranchId.substr(2));
			} catch (...) {}
		}
		char branchTag[16];
		std::snprintf(branchTag, sizeof(branchTag), "%02d", branchNum);
		numDigits = branchTag;

		// 1. Automatically create Manager account: manager<branchNum> / mgr<branchNum>pass
		std::string mgrUser = "manager" + std::to_string(branchNum);
		std::string mgrPass = "mgr" + std::to_string(branchNum) + "pass";
		std::string mgrSalt = GenerateSalt();
		std::string mgrHash = HashPassword(mgrPass, mgrSalt);
		std::string mgrId = "MGR" + numDigits;
		std::string mgrName = "Quản lý " + inputBranchName;

		AccountRecord mgrAcc{mgrId, mgrUser, mgrName, "Manager", inputBranchId, mgrSalt, mgrHash, nullptr};
		userPool.push_back(std::make_unique<Manager>(mgrId, mgrName, inputBranchId));
		userPool.back()->setUsername(mgrUser);
		mgrAcc.user = userPool.back().get();
		accounts.push_back(mgrAcc);

		// 2. Automatically create 3 default Employees: employee<branch><num> / emp<branch><num>pass
		for (int i = 1; i <= 3; ++i) {
			char empNumBuf[8];
			std::snprintf(empNumBuf, sizeof(empNumBuf), "%02d", i);
			std::string empUser = "employee" + numDigits + std::string(empNumBuf);
			std::string empPass = "emp" + numDigits + std::string(empNumBuf) + "pass";
			std::string empSalt = GenerateSalt();
			std::string empHash = HashPassword(empPass, empSalt);
			std::string empId = "EMP" + numDigits + std::string(empNumBuf);
			std::string empName = "Nhân viên " + numDigits + "-" + std::string(empNumBuf);

			AccountRecord empAcc{empId, empUser, empName, "Employee", inputBranchId, empSalt, empHash, nullptr};
			userPool.push_back(std::make_unique<Employee>(empId, empName, inputBranchId));
			userPool.back()->setUsername(empUser);
			empAcc.user = userPool.back().get();
			accounts.push_back(empAcc);
		}

		SaveAccountsToFile(accounts);

		statusVersion++;
		statusMessage = "Đã thêm " + inputBranchName + " (" + inputBranchId + ")! Tự động tạo 1 Quản lý (" + mgrUser + ") & 3 Nhân viên.";
	} else if (branchModalMode == 1 && editingBranchIdx >= 0 && editingBranchIdx < static_cast<int>(branches.size())) {
		branches[editingBranchIdx] = Branch(inputBranchId, inputBranchName,
			static_cast<float>(inputBranchGridX), static_cast<float>(inputBranchGridY));
		branches[editingBranchIdx].setOpeningHours("08:00", "22:30");
		statusVersion++;
		statusMessage = "Đã cập nhật thông tin chi nhánh: " + inputBranchName;
	}

	isBranchModalOpen = false;
	return true;
}

void DutMapUI::deleteBranch(int branchIdx, std::vector<Branch>& branches,
	std::vector<AccountRecord>& accounts, std::vector<std::unique_ptr<User>>& /*userPool*/,
	std::string& statusMessage, int& statusVersion) {
	if (branches.size() <= 1) {
		statusVersion++;
		statusMessage = "Không thể xoá chi nhánh cuối cùng của hệ thống!";
		return;
	}
	if (branchIdx < 0 || branchIdx >= static_cast<int>(branches.size())) return;

	std::string delId = branches[branchIdx].getId();
	std::string delName = branches[branchIdx].getName();
	branches.erase(branches.begin() + branchIdx);

	// Remove associated accounts
	accounts.erase(std::remove_if(accounts.begin(), accounts.end(),
		[&delId](const AccountRecord& a) { return a.branchId == delId && a.role != "BranchManager"; }), accounts.end());
	SaveAccountsToFile(accounts);

	startBranchIdx = std::clamp(startBranchIdx, 0, static_cast<int>(branches.size()) - 1);
	targetBranchIdx = std::clamp(targetBranchIdx, 0, static_cast<int>(branches.size()) - 1);

	if (startBranchIdx == targetBranchIdx && branches.size() > 1) {
		targetBranchIdx = (startBranchIdx + 1) % branches.size();
		targetPoint = {std::clamp(static_cast<int>(branches[targetBranchIdx].getX()), 0, DutMap::COLS - 1),
		               std::clamp(static_cast<int>(branches[targetBranchIdx].getY()), 0, DutMap::ROWS - 1)};
		isTargetCustom = false;
	}

	if (!branches.empty()) {
		int sx = std::clamp(static_cast<int>(branches[startBranchIdx].getX()), 0, DutMap::COLS - 1);
		int sy = std::clamp(static_cast<int>(branches[startBranchIdx].getY()), 0, DutMap::ROWS - 1);
		int tx = std::clamp(targetPoint.x, 0, DutMap::COLS - 1);
		int ty = std::clamp(targetPoint.y, 0, DutMap::ROWS - 1);
		currentRoute = map.findRoute(map.nodeAt(sx, sy), map.nodeAt(tx, ty), useAStar);
		pathProgress = 0.0f;
		shipperProgress = 0.0f;
	}

	statusVersion++;
	statusMessage = "Đã xoá chi nhánh: " + delName + " (" + delId + ") và thu hồi các tài khoản liên quan.";
}

void DutMapUI::handleScroll(Vector2 mouse, float wheelMove) {
	if (std::abs(wheelMove) < 0.01f || isBranchModalOpen) return;
	int step = (wheelMove > 0) ? -1 : 1;
	if (CheckCollisionPointRec(mouse, {980, 540, 275, 120})) {
		branchListScroll = std::max(0, branchListScroll + step);
	}
}

bool DutMapUI::handleInput(int key, int charPressed, std::vector<Branch>& branches,
	std::vector<AccountRecord>& accounts, std::vector<std::unique_ptr<User>>& userPool,
	User* /*currentUser*/, std::string& statusMessage, int& statusVersion) {
	if (!isBranchModalOpen) return false;

	if (key == KEY_ESCAPE) {
		isBranchModalOpen = false;
		return true;
	}
	if (key == KEY_ENTER) {
		bool ok = submitBranchModal(branches, accounts, userPool, statusMessage, statusVersion);
		if (ok && !branches.empty()) {
			startBranchIdx = std::clamp(startBranchIdx, 0, static_cast<int>(branches.size()) - 1);
			int sx = std::clamp(static_cast<int>(branches[startBranchIdx].getX()), 0, DutMap::COLS - 1);
			int sy = std::clamp(static_cast<int>(branches[startBranchIdx].getY()), 0, DutMap::ROWS - 1);
			int tx = std::clamp(targetPoint.x, 0, DutMap::COLS - 1);
			int ty = std::clamp(targetPoint.y, 0, DutMap::ROWS - 1);
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

	std::string* targetStr = nullptr;
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
			coord = std::clamp(coord, 0, (branchModalField == 2) ? (DutMap::COLS - 1) : (DutMap::ROWS - 1));
			return true;
		}
	}
	return false;
}

bool DutMapUI::handleClick(Vector2 mouse, std::vector<Branch>& branches,
	std::vector<AccountRecord>& accounts, std::vector<std::unique_ptr<User>>& userPool,
	User* currentUser, std::string& statusMessage, int& statusVersion) {
	bool isBranchMgr = currentUser && currentUser->canManageAllBranches();

	// Modal handling
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
			bool ok = submitBranchModal(branches, accounts, userPool, statusMessage, statusVersion);
			if (ok && !branches.empty()) {
				startBranchIdx = std::clamp(startBranchIdx, 0, static_cast<int>(branches.size()) - 1);
				int sx = std::clamp(static_cast<int>(branches[startBranchIdx].getX()), 0, DutMap::COLS - 1);
				int sy = std::clamp(static_cast<int>(branches[startBranchIdx].getY()), 0, DutMap::ROWS - 1);
				int tx = std::clamp(targetPoint.x, 0, DutMap::COLS - 1);
				int ty = std::clamp(targetPoint.y, 0, DutMap::ROWS - 1);
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

	// 1. Check clicks on Map Area
	Rectangle mapRec = {mapOriginX, mapOriginY, DutMap::COLS * mapCellSize, DutMap::ROWS * mapCellSize};
	if (CheckCollisionPointRec(mouse, mapRec)) {
		int cellX = static_cast<int>((mouse.x - mapOriginX) / mapCellSize);
		int cellY = static_cast<int>((mouse.y - mapOriginY) / mapCellSize);
		cellX = std::clamp(cellX, 0, DutMap::COLS - 1);
		cellY = std::clamp(cellY, 0, DutMap::ROWS - 1);

		// Check if click was on a branch
		int clickedBranch = -1;
		for (int i = 0; i < static_cast<int>(branches.size()); ++i) {
			int bx = std::clamp(static_cast<int>(branches[i].getX()), 0, DutMap::COLS - 1);
			int by = std::clamp(static_cast<int>(branches[i].getY()), 0, DutMap::ROWS - 1);
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
			// Click 1: Select start branch
			if (clickedBranch >= 0) {
				startBranchIdx = clickedBranch;
				selectionStep = 1;
				statusVersion++;
				statusMessage = "Đã chọn điểm xuất phát: " + branches[startBranchIdx].getName() + ". Hãy chọn điểm đến!";
			} else {
				statusVersion++;
				statusMessage = "Vui lòng chọn 1 chi nhánh làm điểm xuất phát!";
				return false;
			}
		} else {
			// Click 2: Select destination
			if (clickedBranch >= 0) {
				if (clickedBranch == startBranchIdx) {
					statusVersion++;
					statusMessage = "Điểm đến trùng với điểm xuất phát. Vui lòng chọn điểm khác!";
					return false;
				}
				targetBranchIdx = clickedBranch;
				isTargetCustom = false;
				int tbx = std::clamp(static_cast<int>(branches[targetBranchIdx].getX()), 0, DutMap::COLS - 1);
				int tby = std::clamp(static_cast<int>(branches[targetBranchIdx].getY()), 0, DutMap::ROWS - 1);
				targetPoint = {tbx, tby};
				statusVersion++;
				statusMessage = "Đã chọn điểm đến: " + branches[targetBranchIdx].getName();
			} else {
				// Custom address / street
				isTargetCustom = true;
				targetBranchIdx = -1;
				targetPoint = {cellX, cellY};
				statusVersion++;
				statusMessage = "Đã chọn địa chỉ giao hàng tại toạ độ (" + std::to_string(cellX) + ", " + std::to_string(cellY) + ")";
			}
		}

		// Recompute route
		if (!branches.empty()) {
			startBranchIdx = std::clamp(startBranchIdx, 0, static_cast<int>(branches.size()) - 1);
			int sx = std::clamp(static_cast<int>(branches[startBranchIdx].getX()), 0, DutMap::COLS - 1);
			int sy = std::clamp(static_cast<int>(branches[startBranchIdx].getY()), 0, DutMap::ROWS - 1);
			int startNode = map.nodeAt(sx, sy);
			int targetNode = map.nodeAt(targetPoint.x, targetPoint.y);
			currentRoute = map.findRoute(startNode, targetNode, useAStar);
			pathProgress = 0.0f;
			shipperProgress = 0.0f;
		}
		return false;
	}

	// 2. Control buttons in Right Panel
	const float px = 995.0f;
	const float pw = 245.0f;

	// Reset / Choose Start button
	Rectangle resetStartBtn = {px, 312, pw, 32};
	if (CheckCollisionPointRec(mouse, resetStartBtn)) {
		selectionStep = 0;
		statusVersion++;
		statusMessage = "Hãy nhấp vào 1 chi nhánh trên bản đồ để chọn làm điểm xuất phát mới!";
		return false;
	}

	// Toggle Algorithm
	Rectangle algoBtn = {px, 352, pw, 32};
	if (CheckCollisionPointRec(mouse, algoBtn)) {
		useAStar = !useAStar;
		pathProgress = 0.0f;
		shipperProgress = 0.0f;
		if (!branches.empty()) {
			startBranchIdx = std::clamp(startBranchIdx, 0, static_cast<int>(branches.size()) - 1);
			int sx = std::clamp(static_cast<int>(branches[startBranchIdx].getX()), 0, DutMap::COLS - 1);
			int sy = std::clamp(static_cast<int>(branches[startBranchIdx].getY()), 0, DutMap::ROWS - 1);
			int startNode = map.nodeAt(sx, sy);
			int targetNode = map.nodeAt(targetPoint.x, targetPoint.y);
			currentRoute = map.findRoute(startNode, targetNode, useAStar);
		}
		statusVersion++;
		statusMessage = useAStar ? "Đã chuyển sang thuật toán A* (Tối ưu)" : "Đã chuyển sang thuật toán Dijkstra";
		return false;
	}

	// 3. Branch Management Section (BranchManager only)
	if (isBranchMgr) {
		Rectangle addBrBtn = {px, 478, pw, 28};
		if (CheckCollisionPointRec(mouse, addBrBtn)) {
			openAddBranchModal(branches);
			return false;
		}

		// Branch rows in small list
		const float branchRowStartY = 514.0f;
		const float branchRowHeight = 26.0f;
		for (int i = 0; i < static_cast<int>(branches.size()); ++i) {
			float rowY = branchRowStartY + i * branchRowHeight;
			if (rowY > 690) break;
			Rectangle editBtn = {px + pw - 74, rowY, 34, 20};
			Rectangle delBtn = {px + pw - 36, rowY, 34, 20};
			if (CheckCollisionPointRec(mouse, editBtn)) {
				openEditBranchModal(i, branches);
				return false;
			}
			if (CheckCollisionPointRec(mouse, delBtn)) {
				deleteBranch(i, branches, accounts, userPool, statusMessage, statusVersion);
				return true;
			}
		}
	}
	return false;
}

void DutMapUI::draw(Vector2 mouse, const std::vector<Branch>& branches,
	OrderQueue& orderQueue, User* currentUser) {
	(void)orderQueue;
	bool isBranchMgr = currentUser && currentUser->canManageAllBranches();
	float dt = Anim::Dt();
	pathProgress += dt * 140.0f;

	int totalExplored = static_cast<int>(currentRoute.explored.size());
	int expCount = std::min(totalExplored, static_cast<int>(pathProgress));
	bool searchFinished = (pathProgress >= static_cast<float>(totalExplored));

	// Header
	DrawUiText("Bản Đồ Khu Vực ĐH Bách Khoa & Tìm Đường", 245, 104, 24, Color{35, 55, 53, 255});
	DrawUiTextFitted("Khu vực: ĐH Bách Khoa - Chợ Hoà Khánh - Ngô Sĩ Liên - Âu Cơ  •  Nhấp chọn 2 điểm để tìm lộ trình tối ưu",
		245, 136, 1000, 13, Color{104, 115, 108, 255});

	// Draw Background Terrain & Street Grid
	float mapW = DutMap::COLS * mapCellSize;
	float mapH = DutMap::ROWS * mapCellSize;
	DrawRectangle(static_cast<int>(mapOriginX), static_cast<int>(mapOriginY), static_cast<int>(mapW), static_cast<int>(mapH), Color{236, 233, 222, 255});

	for (int y = 0; y < DutMap::ROWS; ++y) {
		for (int x = 0; x < DutMap::COLS; ++x) {
			Rectangle cell = {mapOriginX + x * mapCellSize, mapOriginY + y * mapCellSize, mapCellSize, mapCellSize};
			DutCellType type = map.getCellType(x, y);

			switch (type) {
			case DutCellType::MainHighway:
				DrawRectangleRec(cell, Color{55, 62, 66, 255}); // Dark highway asphalt
				// Yellow dashed divider between column 11 and 12
				if (x == 11 && (y % 2 == 0)) {
					DrawRectangle(static_cast<int>(cell.x + cell.width - 1), static_cast<int>(cell.y + 4), 2, 16, Color{250, 218, 94, 230});
				}
				break;
			case DutCellType::Road:
				DrawRectangleRec(cell, Color{75, 82, 86, 255}); // Standard asphalt
				break;
			case DutCellType::Campus:
				DrawRectangleRec(cell, Color{216, 236, 214, 255}); // Green campus lawns
				DrawCircle(static_cast<int>(cell.x + 6), static_cast<int>(cell.y + 6), 3, Color{145, 195, 140, 180});
				break;
			case DutCellType::CampusBuilding:
				DrawRectangleRounded(cell, 0.15f, 4, Color{180, 75, 65, 255}); // University building brick
				DrawRectangleLines(static_cast<int>(cell.x), static_cast<int>(cell.y), static_cast<int>(cell.width), static_cast<int>(cell.height), Color{140, 50, 40, 255});
				break;
			case DutCellType::Market:
				DrawRectangleRec(cell, Color{242, 185, 130, 255}); // Market stalls terracotta
				break;
			case DutCellType::Water:
				DrawRectangleRec(cell, Color{90, 155, 205, 255}); // Lake blue
				// subtle water ripples
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

	// Landmark Watermark Labels on the Map
	DrawUiText("TRƯỜNG ĐẠI HỌC BÁCH KHOA (DUT)", mapOriginX + 13 * mapCellSize + 4, mapOriginY + 0.5f * mapCellSize, 11, Color{30, 85, 55, 230});
	DrawUiText("CHỢ HOÀ KHÁNH", mapOriginX + 7 * mapCellSize + 4, mapOriginY + 11.2f * mapCellSize, 10, Color{130, 65, 30, 240});
	DrawUiText("HỒ BÀU TRÀM", mapOriginX + 26 * mapCellSize + 4, mapOriginY + 15.0f * mapCellSize, 10, Color{45, 95, 140, 240});

	// Street Names
	DrawUiText("<- ÂU CƠ (BÀ NÀ)", mapOriginX + 1 * mapCellSize, mapOriginY + 13.2f * mapCellSize, 9, Color{245, 245, 245, 220});
	DrawUiText("NGÔ SĨ LIÊN (KTX) ->", mapOriginX + 20 * mapCellSize, mapOriginY + 5.2f * mapCellSize, 9, Color{245, 245, 245, 220});
	DrawUiText("QL1A (NGUYỄN LƯƠNG BẰNG)", mapOriginX + 11.1f * mapCellSize, mapOriginY + 19.1f * mapCellSize, 9, Color{250, 225, 110, 240});

	// Draw Explored Nodes (Animation during search)
	for (int i = 0; i < expCount; ++i) {
		int u = currentRoute.explored[i];
		int ux = map.getNodeX(u);
		int uy = map.getNodeY(u);
		Rectangle r = {mapOriginX + ux * mapCellSize + 7, mapOriginY + uy * mapCellSize + 7, 10, 10};
		Color expCol = useAStar ? Color{165, 215, 195, 150} : Color{180, 200, 225, 150};
		DrawRectangleRounded(r, 0.4f, 4, expCol);
	}

	// Draw Calculated Path Ribbon and Shipper ONLY AFTER search exploration has completed!
	if (searchFinished && currentRoute.nodes.size() > 1) {
		float routeTime = (pathProgress - static_cast<float>(totalExplored));
		int maxSegments = static_cast<int>(currentRoute.nodes.size()) - 1;
		int visibleSegments = std::clamp(static_cast<int>(routeTime * 4.0f), 0, maxSegments);

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

		// Draw Animated Shipper / Delivery Motorcycle Icon ONLY after path ribbon has fully drawn
		if (visibleSegments >= maxSegments) {
			shipperProgress += dt * 6.0f;
			if (shipperProgress >= static_cast<float>(maxSegments)) {
				shipperProgress = 0.0f;
			}

			int segIdx = std::clamp(static_cast<int>(shipperProgress), 0, maxSegments - 1);
			float segFrac = shipperProgress - static_cast<float>(segIdx);
			int u = currentRoute.nodes[segIdx];
			int v = currentRoute.nodes[segIdx + 1];
			Vector2 p1 = {mapOriginX + map.getNodeX(u) * mapCellSize + mapCellSize * 0.5f,
				          mapOriginY + map.getNodeY(u) * mapCellSize + mapCellSize * 0.5f};
			Vector2 p2 = {mapOriginX + map.getNodeX(v) * mapCellSize + mapCellSize * 0.5f,
				          mapOriginY + map.getNodeY(v) * mapCellSize + mapCellSize * 0.5f};
			Vector2 curPos = {p1.x + (p2.x - p1.x) * segFrac, p1.y + (p2.y - p1.y) * segFrac};

			// Pulsing Shipper Marker
			float pulse = 0.5f + 0.5f * std::sin(GetTime() * 8.0f);
			DrawCircleV(curPos, 12.0f + 3.0f * pulse, Color{255, 140, 50, static_cast<unsigned char>(100.0f * (1.0f - pulse))});
			DrawCircleV(curPos, 9.0f, Color{235, 95, 30, 255});
			DrawCircleV(curPos, 6.0f, RAYWHITE);
			DrawCircleV(curPos, 3.5f, Color{35, 55, 53, 255});
		}
	}

	// Draw Branches as Map Badges
	for (int i = 0; i < static_cast<int>(branches.size()); ++i) {
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

		// Name tag
		std::string tag = branches[i].getName();
		if (isStart) tag = "[XUẤT PHÁT] " + tag;
		else if (isDest) tag = "[ĐÍCH ĐẾN] " + tag;

		float tagW = MeasureUiText(tag.c_str(), 11) + 12.0f;
		float tagX = std::clamp(px - tagW * 0.5f, mapOriginX + 4.0f, mapOriginX + mapW - tagW - 4.0f);
		float tagY = std::clamp(py + 12.0f, mapOriginY + 4.0f, mapOriginY + mapH - 22.0f);
		Rectangle tagRec = {tagX, tagY, tagW, 18};
		DrawRectangleRounded(tagRec, 0.3f, 4, (isStart || isDest) ? pinCol : Color{255, 255, 255, 235});
		DrawUiTextInRect(tag.c_str(), tagRec, 11, (isStart || isDest) ? RAYWHITE : Color{35, 55, 53, 255}, 0, 4.0f);
	}

	// Draw Target Pin if custom address
	if (isTargetCustom) {
		float tx = mapOriginX + targetPoint.x * mapCellSize + mapCellSize * 0.5f;
		float ty = mapOriginY + targetPoint.y * mapCellSize + mapCellSize * 0.5f;
		DrawCircle(static_cast<int>(tx), static_cast<int>(ty), 10.0f, Color{225, 75, 45, 255});
		DrawCircle(static_cast<int>(tx), static_cast<int>(ty), 4.0f, RAYWHITE);

		std::string destTag = "[GIAO TẬN NƠI]";
		float dtw = MeasureUiText(destTag.c_str(), 11) + 12.0f;
		Rectangle drec = {tx - dtw * 0.5f, ty + 12.0f, dtw, 18};
		DrawRectangleRounded(drec, 0.3f, 4, Color{225, 75, 45, 255});
		DrawUiTextInRect(destTag.c_str(), drec, 11, RAYWHITE, 0, 4.0f);
	}

	// Right Panel: Route information & Branch management
	Rectangle rightPanel = {980, 168, 275, 540};
	DrawRectangleRounded(rightPanel, 0.05f, 4, RAYWHITE);
	DrawRectangleRoundedLines(rightPanel, 0.05f, 4, Color{220, 228, 221, 255});

	const float rpx = 995.0f;
	const float rpw = 245.0f;

	DrawUiTextFitted(useAStar ? "THUẬT TOÁN A* (TỐI ƯU)" : "THUẬT TOÁN DIJKSTRA", rpx, 186, rpw, 13,
		useAStar ? Color{215, 105, 30, 255} : Color{35, 125, 75, 255});

	std::string startName = (startBranchIdx >= 0 && startBranchIdx < static_cast<int>(branches.size()))
		? branches[startBranchIdx].getName() : "Chưa chọn";
	std::string targetName = isTargetCustom
		? ("Toạ độ (" + std::to_string(targetPoint.x) + ", " + std::to_string(targetPoint.y) + ")")
		: ((targetBranchIdx >= 0 && targetBranchIdx < static_cast<int>(branches.size()))
			? branches[targetBranchIdx].getName() : "Chưa chọn");

	DrawUiTextFitted(("1. Xuất phát: " + startName).c_str(), rpx, 208, rpw, 13, Color{35, 135, 85, 255});
	DrawUiTextFitted(("2. Điểm đến: " + targetName).c_str(), rpx, 230, rpw, 13, Color{215, 85, 45, 255});

	if (currentRoute.nodes.size() > 1) {
		float distanceKm = currentRoute.distance * 0.12f; // scale each cell to approx 120m
		int estMinutes = std::max(2, static_cast<int>(distanceKm * 3.5f + 2.0f));
		char statBuf[100];
		std::snprintf(statBuf, sizeof(statBuf), "Quãng đường: %.1f km (%d ô)  |  Dự kiến: %d phút",
			distanceKm, static_cast<int>(currentRoute.nodes.size()), estMinutes);
		DrawUiTextFitted(statBuf, rpx, 256, rpw, 12, Color{54, 66, 58, 255});
	} else {
		DrawUiTextFitted("Quãng đường: -- km  |  Dự kiến: --", rpx, 256, rpw, 12, Color{120, 130, 125, 255});
	}

	// Status badge
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

	// Action buttons
	Rectangle resetStartBtn = {rpx, 312, rpw, 32};
	Anim::Button(resetStartBtn, Color{235, 240, 235, 255}, mouse, true, 0.2f);
	DrawUiTextInRect(selectionStep == 0 ? "-> Hãy click CN xuất phát..." : "Đổi điểm xuất phát",
		resetStartBtn, 12, Color{35, 105, 78, 255}, 0, 4.0f);

	Rectangle algoBtn = {rpx, 352, rpw, 32};
	Anim::Button(algoBtn, Color{224, 231, 220, 255}, mouse, true, 0.2f);
	DrawUiTextInRect(useAStar ? "Đổi sang: Thuật toán Dijkstra" : "Đổi sang: Thuật toán A*",
		algoBtn, 12, Color{54, 66, 58, 255}, 0, 6.0f);

	// Algorithm explanation card
	Rectangle infoCard = {rpx, 392, rpw, 46};
	DrawRectangleRounded(infoCard, 0.15f, 4, Color{242, 245, 241, 255});
	DrawRectangleRoundedLines(infoCard, 0.15f, 4, Color{215, 224, 218, 255});
	if (useAStar) {
		DrawUiTextFitted("Thuật toán A* (Heuristic hướng đích)", rpx + 8, 396, rpw - 16, 11, Color{215, 105, 30, 255});
		DrawUiTextFitted("Kết hợp chi phí thực tế & khoảng cách Manhattan", rpx + 8, 410, rpw - 16, 10, Color{85, 102, 92, 255});
		DrawUiTextFitted("định hướng trực tiếp tới đích, duyệt ít ô hơn.", rpx + 8, 423, rpw - 16, 10, Color{85, 102, 92, 255});
	} else {
		DrawUiTextFitted("Thuật toán Dijkstra (Quét lan toả)", rpx + 8, 396, rpw - 16, 11, Color{35, 125, 75, 255});
		DrawUiTextFitted("Duyệt đều theo hình gợn sóng mở rộng mọi hướng,", rpx + 8, 410, rpw - 16, 10, Color{85, 102, 92, 255});
		DrawUiTextFitted("đảm bảo tìm thấy đường ngắn nhất tuyệt đối.", rpx + 8, 423, rpw - 16, 10, Color{85, 102, 92, 255});
	}

	// Separator
	DrawLine(static_cast<int>(rpx), 448, static_cast<int>(rpx + rpw), 448, Color{225, 232, 226, 255});

	// Branch Management Section
	if (isBranchMgr) {
		DrawUiText("QUẢN LÝ CHI NHÁNH (GĐ CHUỖI)", rpx, 458, 12, Color{35, 55, 53, 255});

		Rectangle addBrBtn = {rpx, 478, rpw, 28};
		Anim::Button(addBrBtn, Color{35, 105, 78, 255}, mouse, true, 0.2f);
		DrawUiTextInRect("+ THÊM CHI NHÁNH MỚI", addBrBtn, 11, RAYWHITE, 0, 4.0f);

		const float branchRowStartY = 514.0f;
		const float branchRowHeight = 26.0f;
		for (int i = 0; i < static_cast<int>(branches.size()); ++i) {
			float rowY = branchRowStartY + i * branchRowHeight;
			if (rowY > 690) break;
			std::string binfo = branches[i].getId() + " - " + branches[i].getName();
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
		for (int i = 0; i < static_cast<int>(branches.size()); ++i) {
			float rowY = branchRowStartY + i * branchRowHeight;
			if (rowY > 690) break;
			std::string binfo = branches[i].getId() + " - " + branches[i].getName();
			DrawUiTextFitted(binfo.c_str(), rpx, rowY + 3, rpw, 11, Color{45, 56, 50, 255});
		}
	}

	// Modal Dialog: Add / Edit Branch
	if (isBranchModalOpen) {
		DrawRectangle(0, 0, 1280, 760, Color{0, 0, 0, 140});
		Rectangle modal = {400, 150, 480, 420};
		DrawRectangleRounded(modal, 0.05f, 8, RAYWHITE);
		DrawRectangleRoundedLines(modal, 0.05f, 8, Color{35, 105, 78, 255});

		const char* title = branchModalMode == 0 ? "THÊM CHI NHÁNH MỚI" : "CHỈNH SỬA CHI NHÁNH";
		DrawUiTextFitted(title, modal.x + 30, modal.y + 24, 420, 18, Color{35, 55, 53, 255});
		DrawUiTextFitted("Tự động sinh 1 tài khoản Quản lý và 3 Nhân viên khi thêm", modal.x + 30, modal.y + 48, 420, 12, Color{104, 115, 108, 255});

		// Field 0: Branch ID
		DrawUiText("Mã chi nhánh (VD: BR005):", modal.x + 30, modal.y + 70, 12, Color{35, 55, 53, 255});
		Rectangle idBox = {modal.x + 30, modal.y + 90, 420, 36};
		DrawUiInputBox(idBox, inputBranchId, "BR005", branchModalField == 0, mouse);

		// Field 1: Branch Name
		DrawUiText("Tên chi nhánh:", modal.x + 30, modal.y + 138, 12, Color{35, 55, 53, 255});
		Rectangle nameBox = {modal.x + 30, modal.y + 158, 420, 36};
		DrawUiInputBox(nameBox, inputBranchName, "Nhập tên chi nhánh...", branchModalField == 1, mouse);

		// Field 2 & 3: Grid X, Y
		DrawUiText("Toạ độ X trên bản đồ (0..29):", modal.x + 30, modal.y + 208, 12, Color{35, 55, 53, 255});
		Rectangle xBox = {modal.x + 30, modal.y + 228, 200, 36};
		DrawUiInputBox(xBox, std::to_string(inputBranchGridX), "12", branchModalField == 2, mouse);

		DrawUiText("Toạ độ Y trên bản đồ (0..19):", modal.x + 250, modal.y + 208, 12, Color{35, 55, 53, 255});
		Rectangle yBox = {modal.x + 250, modal.y + 228, 200, 36};
		DrawUiInputBox(yBox, std::to_string(inputBranchGridY), "3", branchModalField == 3, mouse);

		DrawUiTextFitted("Nhấn Tab để chuyển ô  •  Enter để Lưu  •  Esc để Huỷ", modal.x + 30, modal.y + 280, 420, 12, Color{104, 115, 108, 255});

		// Action buttons
		Rectangle saveBtn = {modal.x + 30, modal.y + 340, 200, 42};
		Rectangle cancelBtn = {modal.x + 250, modal.y + 340, 200, 42};
		Anim::Button(saveBtn, Color{35, 105, 78, 255}, mouse, true, 0.2f);
		DrawUiTextInRect("LƯU CHI NHÁNH", saveBtn, 13, RAYWHITE, 0, 8.0f);
		Anim::Button(cancelBtn, Color{150, 160, 152, 255}, mouse, true, 0.2f);
		DrawUiTextInRect("HUỶ BỎ", cancelBtn, 13, RAYWHITE, 0, 8.0f);
	}
}
