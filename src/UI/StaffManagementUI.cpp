#include "StaffManagementUI.h"
#include "UIFont.h"
#include "UIAnim.h"

#include <algorithm>
#include <cstdio>
#include <cctype>

StaffManagementUI::StaffManagementUI() {}

void StaffManagementUI::openAddStaffModal(const User& currentUser,
	const std::vector<Branch>& branches, const std::vector<AccountRecord>& accounts) {
	isStaffModalOpen = true;
	staffModalMode = 0;
	editingAccountIdx = -1;

	bool isBranchMgr = currentUser.canManageAllBranches();
	inputStaffRole = "Employee";
	inputStaffBranchId = isBranchMgr ? (branches.empty() ? "BR001" : branches.front().getId()) : currentUser.getBranchId();

	// Extract numeric part of branchId (e.g. BR003 -> "03", 3)
	int brNum = 1;
	std::string brDigits = "01";
	if (inputStaffBranchId.size() >= 3) {
		try {
			brNum = std::stoi(inputStaffBranchId.substr(2));
			char buf[8];
			std::snprintf(buf, sizeof(buf), "%02d", brNum);
			brDigits = buf;
		} catch (...) {}
	}

	// Count existing employees in this branch to determine next index
	int empCount = 0;
	for (const auto& a : accounts) {
		if (a.branchId == inputStaffBranchId && a.role == "Employee") {
			empCount++;
		}
	}
	int nextEmpIdx = empCount + 1;
	char empNumBuf[8];
	std::snprintf(empNumBuf, sizeof(empNumBuf), "%02d", nextEmpIdx);

	// Suggest default username & password: employee<branch><num> / emp<branch><num>pass
	inputStaffUsername = "employee" + brDigits + std::string(empNumBuf);
	inputStaffPassword = "emp" + brDigits + std::string(empNumBuf) + "pass";
	inputStaffId = "EMP" + brDigits + std::string(empNumBuf);
	inputStaffName = "Nhân viên mới " + std::string(empNumBuf);
	staffModalField = 0;
}

void StaffManagementUI::openEditStaffModal(int accountIdx, const std::vector<AccountRecord>& accounts) {
	if (accountIdx < 0 || accountIdx >= static_cast<int>(accounts.size())) return;
	isStaffModalOpen = true;
	staffModalMode = 1;
	editingAccountIdx = accountIdx;

	const auto& acc = accounts[accountIdx];
	inputStaffId = acc.id;
	inputStaffName = acc.name;
	inputStaffUsername = acc.username;
	inputStaffPassword = ""; // leave blank if not changing
	inputStaffBranchId = acc.branchId;
	inputStaffRole = acc.role;
	staffModalField = 0;
}

void StaffManagementUI::openChangePassModal() {
	isChangePassModalOpen = true;
	inputOldPassword = "";
	inputNewPassword = "";
	inputConfirmPassword = "";
	changePassField = 0;
}

bool StaffManagementUI::submitStaffModal(std::vector<AccountRecord>& accounts,
	std::vector<std::unique_ptr<User>>& userPool, const User& /*currentUser*/,
	std::string& statusMessage, int& statusVersion) {
	if (inputStaffName.empty() || inputStaffUsername.empty()) {
		statusVersion++;
		statusMessage = "Vui lòng nhập đầy đủ Họ tên và Tên tài khoản!";
		return false;
	}

	// Check duplicate username
	for (int i = 0; i < static_cast<int>(accounts.size()); ++i) {
		if (staffModalMode == 1 && i == editingAccountIdx) continue;
		if (accounts[i].username == inputStaffUsername) {
			statusVersion++;
			statusMessage = "Tên tài khoản '" + inputStaffUsername + "' đã tồn tại! Vui lòng chọn tên khác.";
			return false;
		}
	}

	if (staffModalMode == 0) {
		// ADD NEW STAFF
		if (inputStaffPassword.empty()) {
			statusVersion++;
			statusMessage = "Vui lòng nhập mật khẩu cho nhân viên!";
			return false;
		}

		std::string salt = GenerateSalt();
		std::string hash = HashPassword(inputStaffPassword, salt);

		AccountRecord acc{inputStaffId, inputStaffUsername, inputStaffName, inputStaffRole, inputStaffBranchId, salt, hash, nullptr};
		if (inputStaffRole == "Manager") {
			userPool.push_back(std::make_unique<Manager>(inputStaffId, inputStaffName, inputStaffBranchId));
		} else {
			userPool.push_back(std::make_unique<Employee>(inputStaffId, inputStaffName, inputStaffBranchId));
		}
		userPool.back()->setUsername(inputStaffUsername);
		acc.user = userPool.back().get();
		accounts.push_back(acc);

		SaveAccountsToFile(accounts);
		statusVersion++;
		statusMessage = "Đã thêm tài khoản " + inputStaffUsername + " (" + inputStaffName + ") thành công!";
	} else if (staffModalMode == 1 && editingAccountIdx >= 0 && editingAccountIdx < static_cast<int>(accounts.size())) {
		// EDIT EXISTING STAFF
		AccountRecord& acc = accounts[editingAccountIdx];
		acc.name = inputStaffName;
		acc.username = inputStaffUsername;
		acc.branchId = inputStaffBranchId;
		acc.role = inputStaffRole;

		// Update linked User object
		if (acc.user) {
			acc.user->setUsername(inputStaffUsername);
		}

		// Update password if entered
		if (!inputStaffPassword.empty()) {
			acc.salt = GenerateSalt();
			acc.passwordHash = HashPassword(inputStaffPassword, acc.salt);
		}

		SaveAccountsToFile(accounts);
		statusVersion++;
		statusMessage = "Đã cập nhật tài khoản: " + inputStaffUsername + " (" + inputStaffName + ")!";
	}

	isStaffModalOpen = false;
	return true;
}

bool StaffManagementUI::submitChangePassModal(std::vector<AccountRecord>& accounts,
	User& currentUser, std::string& statusMessage, int& statusVersion) {
	if (inputOldPassword.empty() || inputNewPassword.empty()) {
		statusVersion++;
		statusMessage = "Vui lòng nhập đầy đủ mật khẩu hiện tại và mật khẩu mới!";
		return false;
	}

	AccountRecord* targetAcc = nullptr;
	for (auto& acc : accounts) {
		if (acc.username == currentUser.username) {
			targetAcc = &acc;
			break;
		}
	}

	if (!targetAcc) {
		statusVersion++;
		statusMessage = "Không tìm thấy thông tin tài khoản hiện tại!";
		return false;
	}

	// Verify old password
	if (HashPassword(inputOldPassword, targetAcc->salt) != targetAcc->passwordHash) {
		statusVersion++;
		statusMessage = "Mật khẩu hiện tại không chính xác!";
		return false;
	}

	if (inputNewPassword.size() < 4) {
		statusVersion++;
		statusMessage = "Mật khẩu mới phải có ít nhất 4 ký tự!";
		return false;
	}

	if (inputNewPassword != inputConfirmPassword) {
		statusVersion++;
		statusMessage = "Xác nhận mật khẩu mới không khớp!";
		return false;
	}

	// Update password
	targetAcc->salt = GenerateSalt();
	targetAcc->passwordHash = HashPassword(inputNewPassword, targetAcc->salt);
	SaveAccountsToFile(accounts);

	isChangePassModalOpen = false;
	statusVersion++;
	statusMessage = "Đổi mật khẩu thành công! Mật khẩu mới đã được cập nhật.";
	return true;
}

void StaffManagementUI::deleteStaff(int accountIdx, std::vector<AccountRecord>& accounts,
	const User& currentUser, std::string& statusMessage, int& statusVersion) {
	if (accountIdx < 0 || accountIdx >= static_cast<int>(accounts.size())) return;

	if (accounts[accountIdx].username == currentUser.username) {
		statusVersion++;
		statusMessage = "Bạn không thể tự xoá tài khoản đang đăng nhập!";
		return;
	}

	std::string deletedUser = accounts[accountIdx].username;
	std::string deletedName = accounts[accountIdx].name;
	accounts.erase(accounts.begin() + accountIdx);
	SaveAccountsToFile(accounts);

	statusVersion++;
	statusMessage = "Đã xoá tài khoản: " + deletedUser + " (" + deletedName + ")!";
}

void StaffManagementUI::handleScroll(Vector2 /*mouse*/, float wheelMove) {
	if (std::abs(wheelMove) < 0.01f || isModalOpen()) return;
	int step = (wheelMove > 0) ? -1 : 1;
	tableScrollOffset = std::max(0, tableScrollOffset + step);
}

bool StaffManagementUI::handleInput(int key, int charPressed, std::vector<AccountRecord>& accounts,
	std::vector<std::unique_ptr<User>>& userPool, User& currentUser,
	std::string& statusMessage, int& statusVersion) {
	if (isChangePassModalOpen) {
		if (key == KEY_ESCAPE) { isChangePassModalOpen = false; return true; }
		if (key == KEY_ENTER) return submitChangePassModal(accounts, currentUser, statusMessage, statusVersion);
		if (key == KEY_TAB) { changePassField = (changePassField + 1) % 3; return true; }

		std::string* target = (changePassField == 0) ? &inputOldPassword
			: ((changePassField == 1) ? &inputNewPassword : &inputConfirmPassword);
		if (key == KEY_BACKSPACE && !target->empty()) {
			target->pop_back();
			return true;
		}
		if (charPressed >= 32 && charPressed <= 126 && target->size() < 24) {
			target->push_back(static_cast<char>(charPressed));
			return true;
		}
		return false;
	}

	if (isStaffModalOpen) {
		if (key == KEY_ESCAPE) { isStaffModalOpen = false; return true; }
		if (key == KEY_ENTER) return submitStaffModal(accounts, userPool, currentUser, statusMessage, statusVersion);
		if (key == KEY_TAB) { staffModalField = (staffModalField + 1) % 4; return true; }

		std::string* target = nullptr;
		if (staffModalField == 0) target = &inputStaffName;
		else if (staffModalField == 1) target = &inputStaffUsername;
		else if (staffModalField == 2) target = &inputStaffPassword;

		if (target) {
			if (key == KEY_BACKSPACE && !target->empty()) {
				PopUtf8Char(*target);
				return true;
			}
			if (charPressed >= 32) {
				if (staffModalField == 1 || staffModalField == 2) {
					if (charPressed <= 126 && target->size() < 24) {
						target->push_back(static_cast<char>(charPressed));
					}
				} else {
					AppendUtf8Codepoint(*target, charPressed, 36);
				}
				return true;
			}
		}
		return false;
	}
	return false;
}

bool StaffManagementUI::handleClick(Vector2 mouse, std::vector<AccountRecord>& accounts,
	std::vector<std::unique_ptr<User>>& userPool, const std::vector<Branch>& branches,
	User& currentUser, std::string& statusMessage, int& statusVersion) {
	bool isBranchMgr = currentUser.canManageAllBranches();

	// 1. Change Password Modal
	if (isChangePassModalOpen) {
		Rectangle modal = {400, 180, 480, 360};
		if (!CheckCollisionPointRec(mouse, modal)) {
			isChangePassModalOpen = false;
			return false;
		}
		if (CheckCollisionPointRec(mouse, {modal.x + 30, modal.y + 85, 420, 36})) changePassField = 0;
		if (CheckCollisionPointRec(mouse, {modal.x + 30, modal.y + 155, 420, 36})) changePassField = 1;
		if (CheckCollisionPointRec(mouse, {modal.x + 30, modal.y + 225, 420, 36})) changePassField = 2;

		if (CheckCollisionPointRec(mouse, {modal.x + 30, modal.y + 290, 200, 40})) {
			return submitChangePassModal(accounts, currentUser, statusMessage, statusVersion);
		}
		if (CheckCollisionPointRec(mouse, {modal.x + 250, modal.y + 290, 200, 40})) {
			isChangePassModalOpen = false;
			return false;
		}
		return false;
	}

	// 2. Staff Modal (Add / Edit)
	if (isStaffModalOpen) {
		Rectangle modal = {380, 130, 520, 480};
		if (!CheckCollisionPointRec(mouse, modal)) {
			isStaffModalOpen = false;
			return false;
		}
		if (CheckCollisionPointRec(mouse, {modal.x + 30, modal.y + 80, 460, 36})) staffModalField = 0;
		if (CheckCollisionPointRec(mouse, {modal.x + 30, modal.y + 148, 460, 36})) staffModalField = 1;
		if (CheckCollisionPointRec(mouse, {modal.x + 30, modal.y + 216, 460, 36})) staffModalField = 2;

		// Role pills (BranchManager only)
		if (isBranchMgr) {
			if (CheckCollisionPointRec(mouse, {modal.x + 30, modal.y + 284, 130, 32})) inputStaffRole = "Employee";
			if (CheckCollisionPointRec(mouse, {modal.x + 170, modal.y + 284, 130, 32})) inputStaffRole = "Manager";

			// Branch selector pills
			for (int b = 0; b < static_cast<int>(branches.size()) && b < 4; ++b) {
				Rectangle bp = {modal.x + 30 + b * 110.0f, modal.y + 352, 100, 30};
				if (CheckCollisionPointRec(mouse, bp)) {
					inputStaffBranchId = branches[b].getId();
				}
			}
		}

		if (CheckCollisionPointRec(mouse, {modal.x + 30, modal.y + 415, 220, 42})) {
			return submitStaffModal(accounts, userPool, currentUser, statusMessage, statusVersion);
		}
		if (CheckCollisionPointRec(mouse, {modal.x + 270, modal.y + 415, 220, 42})) {
			isStaffModalOpen = false;
			return false;
		}
		return false;
	}

	// 3. Main Staff Management Page
	// Add Staff Button
	Rectangle addStaffBtn = {1040, 154, 205, 34};
	if (CheckCollisionPointRec(mouse, addStaffBtn)) {
		openAddStaffModal(currentUser, branches, accounts);
		return false;
	}

	// Filter pills for BranchManager
	if (isBranchMgr) {
		// All branches pill
		if (CheckCollisionPointRec(mouse, {245, 154, 100, 32})) selectedBranchFilter = -1;
		for (int b = 0; b < static_cast<int>(branches.size()) && b < 5; ++b) {
			Rectangle bp = {355.0f + b * 95.0f, 154.0f, 88.0f, 32.0f};
			if (CheckCollisionPointRec(mouse, bp)) {
				selectedBranchFilter = b;
			}
		}
	}

	// Table action buttons (Edit & Delete)
	std::vector<int> visibleIdx;
	for (int i = 0; i < static_cast<int>(accounts.size()); ++i) {
		const auto& a = accounts[i];
		if (!isBranchMgr) {
			if (a.branchId != currentUser.getBranchId() || a.role != "Employee") continue;
		} else {
			if (selectedBranchFilter >= 0 && selectedBranchFilter < static_cast<int>(branches.size())) {
				if (a.branchId != branches[selectedBranchFilter].getId()) continue;
			}
		}
		visibleIdx.push_back(i);
	}

	const int visRows = 8;
	int maxScroll = std::max(0, static_cast<int>(visibleIdx.size()) - visRows);
	tableScrollOffset = std::clamp(tableScrollOffset, 0, maxScroll);

	int rowY = 236;
	for (int r = 0; r < visRows; ++r) {
		int pos = tableScrollOffset + r;
		if (pos >= static_cast<int>(visibleIdx.size())) break;
		int accIdx = visibleIdx[pos];

		Rectangle editBtn = {1050, static_cast<float>(rowY + 6), 84, 28};
		Rectangle delBtn = {1144, static_cast<float>(rowY + 6), 84, 28};

		if (CheckCollisionPointRec(mouse, editBtn)) {
			openEditStaffModal(accIdx, accounts);
			return false;
		}
		if (CheckCollisionPointRec(mouse, delBtn)) {
			deleteStaff(accIdx, accounts, currentUser, statusMessage, statusVersion);
			return true;
		}
		rowY += 46;
	}

	return false;
}

void StaffManagementUI::draw(Vector2 mouse, const std::vector<AccountRecord>& accounts,
	const std::vector<Branch>& branches, User& currentUser) {
	bool isBranchMgr = currentUser.canManageAllBranches();

	DrawUiText("Quản lý Nhân Sự & Tài Khoản", 245, 104, 24, Color{35, 55, 53, 255});
	std::string subTitle = isBranchMgr
		? "Quản lý tài khoản, phân quyền và mật khẩu cho toàn bộ Quản lý & Nhân viên hệ thống"
		: ("Quản lý danh sách và tài khoản nhân viên thuộc " + currentUser.getBranchId());
	DrawUiTextFitted(subTitle.c_str(), 245, 134, 1000, 13, Color{104, 115, 108, 255});

	// Add Staff Button
	Rectangle addStaffBtn = {1040, 154, 205, 34};
	Anim::Button(addStaffBtn, Color{35, 105, 78, 255}, mouse, true, 0.25f);
	DrawUiTextInRect(isBranchMgr ? "+ THÊM NHÂN SỰ MỚI" : "+ THÊM NHÂN VIÊN", addStaffBtn, 12, RAYWHITE, 0, 6.0f);

	// Filter bar for BranchManager
	if (isBranchMgr) {
		Rectangle allPill = {245, 154, 100, 32};
		Anim::Button(allPill, selectedBranchFilter == -1 ? Color{35, 105, 78, 255} : Color{232, 238, 233, 255}, mouse, true, 0.25f);
		DrawUiTextInRect("Tất cả CN", allPill, 12, selectedBranchFilter == -1 ? RAYWHITE : Color{35, 55, 53, 255}, 0, 4.0f);

		for (int b = 0; b < static_cast<int>(branches.size()) && b < 5; ++b) {
			Rectangle bp = {355.0f + b * 95.0f, 154.0f, 88.0f, 32.0f};
			bool active = (selectedBranchFilter == b);
			Anim::Button(bp, active ? Color{35, 105, 78, 255} : Color{232, 238, 233, 255}, mouse, true, 0.25f);
			DrawUiTextInRect(branches[b].getId().c_str(), bp, 12, active ? RAYWHITE : Color{35, 55, 53, 255}, 0, 4.0f);
		}
	}

	// Filter accounts
	std::vector<int> visibleIdx;
	for (int i = 0; i < static_cast<int>(accounts.size()); ++i) {
		const auto& a = accounts[i];
		if (!isBranchMgr) {
			if (a.branchId != currentUser.getBranchId() || a.role != "Employee") continue;
		} else {
			if (selectedBranchFilter >= 0 && selectedBranchFilter < static_cast<int>(branches.size())) {
				if (a.branchId != branches[selectedBranchFilter].getId()) continue;
			}
		}
		visibleIdx.push_back(i);
	}

	// Table Header
	int tableY = 198;
	DrawRectangle(245, tableY, 985, 34, Color{232, 238, 233, 255});
	DrawUiText("MÃ NV", 265, tableY + 9, 12, Color{35, 55, 53, 255});
	DrawUiText("HỌ VÀ TÊN", 380, tableY + 9, 12, Color{35, 55, 53, 255});
	DrawUiText("TÀI KHOẢN (USERNAME)", 575, tableY + 9, 12, Color{35, 55, 53, 255});
	DrawUiText("VAI TRÒ", 780, tableY + 9, 12, Color{35, 55, 53, 255});
	DrawUiText("CHI NHÁNH", 915, tableY + 9, 12, Color{35, 55, 53, 255});
	DrawUiText("THAO TÁC", 1080, tableY + 9, 12, Color{35, 55, 53, 255});

	const int visRows = 8;
	int total = static_cast<int>(visibleIdx.size());
	int maxScroll = std::max(0, total - visRows);
	tableScrollOffset = std::clamp(tableScrollOffset, 0, maxScroll);

	int rowY = tableY + 38;
	for (int r = 0; r < visRows; ++r) {
		int pos = tableScrollOffset + r;
		if (pos >= total) break;
		const auto& acc = accounts[visibleIdx[pos]];

		Rectangle rowRec = {245, static_cast<float>(rowY), 985, 42};
		DrawRectangleRec(rowRec, (r % 2 == 0) ? RAYWHITE : Color{248, 250, 248, 255});
		DrawRectangleLines(245, rowY, 985, 42, Color{230, 235, 230, 255});

		// ID Badge
		Rectangle idBadge = {260, static_cast<float>(rowY + 8), 90, 26};
		DrawRectangleRounded(idBadge, 0.2f, 4, Color{238, 243, 240, 255});
		DrawUiTextInRect(acc.id.c_str(), idBadge, 12, Color{35, 55, 53, 255}, 0, 4.0f);

		DrawUiTextFitted(acc.name.c_str(), 380, rowY + 12, 180, 13, Color{45, 56, 50, 255});
		DrawUiTextFitted(acc.username.c_str(), 575, rowY + 12, 190, 13, Color{35, 105, 78, 255});

		// Role pill
		Rectangle rolePill = {775, static_cast<float>(rowY + 8), 115, 26};
		Color roleCol = (acc.role == "BranchManager") ? Color{195, 65, 50, 255}
			: ((acc.role == "Manager") ? Color{180, 110, 40, 255} : Color{60, 95, 75, 255});
		DrawRectangleRounded(rolePill, 0.25f, 4, Color{238, 243, 240, 255});
		DrawUiTextInRect(acc.role.c_str(), rolePill, 12, roleCol, 0, 4.0f);

		DrawUiTextFitted(acc.branchId.c_str(), 915, rowY + 12, 100, 13, Color{104, 115, 108, 255});

		// Action buttons: Sửa / Xoá
		Rectangle editBtn = {1050, static_cast<float>(rowY + 6), 84, 28};
		Rectangle delBtn = {1144, static_cast<float>(rowY + 6), 84, 28};
		Anim::Button(editBtn, Color{180, 110, 40, 255}, mouse, true, 0.2f);
		DrawUiTextInRect("Sửa", editBtn, 12, RAYWHITE, 0, 4.0f);

		bool canDelete = (acc.username != currentUser.username);
		Anim::Button(delBtn, Color{195, 65, 50, 255}, mouse, canDelete, 0.2f);
		DrawUiTextInRect("Xoá", delBtn, 12, RAYWHITE, 0, 4.0f);

		rowY += 46;
	}

	DrawVerticalScrollbar({1236, static_cast<float>(tableY + 38), 8, static_cast<float>(visRows * 46 - 4)},
		total, visRows, tableScrollOffset);

	// Modal Dialog: Add / Edit Staff
	if (isStaffModalOpen) {
		DrawRectangle(0, 0, 1280, 760, Color{0, 0, 0, 140});
		Rectangle modal = {380, 130, 520, 480};
		DrawRectangleRounded(modal, 0.05f, 8, RAYWHITE);
		DrawRectangleRoundedLines(modal, 0.05f, 8, Color{35, 105, 78, 255});

		const char* title = (staffModalMode == 0) ? "THÊM NHÂN SỰ MỚI" : "CHỈNH SỬA TÀI KHOẢN NHÂN SỰ";
		DrawUiTextFitted(title, modal.x + 30, modal.y + 22, 460, 18, Color{35, 55, 53, 255});
		DrawUiTextFitted("Cho phép chỉnh sửa cả Tài khoản (Username) và Mật khẩu (Password)", modal.x + 30, modal.y + 46, 460, 12, Color{104, 115, 108, 255});

		// Field 0: Name
		DrawUiText("Họ và tên nhân viên:", modal.x + 30, modal.y + 68, 12, Color{35, 55, 53, 255});
		Rectangle nameBox = {modal.x + 30, modal.y + 88, 460, 36};
		DrawUiInputBox(nameBox, inputStaffName, "Nhập họ và tên...", staffModalField == 0, mouse);

		// Field 1: Username
		DrawUiText("Tên tài khoản (Username):", modal.x + 30, modal.y + 134, 12, Color{35, 55, 53, 255});
		Rectangle userBox = {modal.x + 30, modal.y + 154, 460, 36};
		DrawUiInputBox(userBox, inputStaffUsername, "employee0101...", staffModalField == 1, mouse);

		// Field 2: Password
		DrawUiText(staffModalMode == 0 ? "Mật khẩu khởi tạo:" : "Mật khẩu mới (để trống nếu không đổi):",
			modal.x + 30, modal.y + 200, 12, Color{35, 55, 53, 255});
		Rectangle passBox = {modal.x + 30, modal.y + 220, 460, 36};
		DrawUiInputBox(passBox, inputStaffPassword, "Nhập mật khẩu...", staffModalField == 2, mouse);

		// Roles and Branch (for BranchManager)
		if (isBranchMgr) {
			DrawUiText("Vai trò nhân sự:", modal.x + 30, modal.y + 266, 12, Color{35, 55, 53, 255});
			Rectangle empPill = {modal.x + 30, modal.y + 286, 130, 32};
			Rectangle mgrPill = {modal.x + 170, modal.y + 286, 130, 32};
			Anim::Button(empPill, inputStaffRole == "Employee" ? Color{35, 105, 78, 255} : Color{232, 238, 233, 255}, mouse, true, 0.25f);
			DrawUiTextInRect("Nhân viên (Employee)", empPill, 11, inputStaffRole == "Employee" ? RAYWHITE : Color{35, 55, 53, 255}, 0, 4.0f);
			Anim::Button(mgrPill, inputStaffRole == "Manager" ? Color{180, 110, 40, 255} : Color{232, 238, 233, 255}, mouse, true, 0.25f);
			DrawUiTextInRect("Quản lý (Manager)", mgrPill, 11, inputStaffRole == "Manager" ? RAYWHITE : Color{35, 55, 53, 255}, 0, 4.0f);

			DrawUiText("Chi nhánh trực thuộc:", modal.x + 30, modal.y + 328, 12, Color{35, 55, 53, 255});
			for (int b = 0; b < static_cast<int>(branches.size()) && b < 4; ++b) {
				Rectangle bp = {modal.x + 30 + b * 110.0f, modal.y + 348, 100, 30};
				bool active = (inputStaffBranchId == branches[b].getId());
				Anim::Button(bp, active ? Color{35, 105, 78, 255} : Color{232, 238, 233, 255}, mouse, true, 0.25f);
				DrawUiTextInRect(branches[b].getId().c_str(), bp, 12, active ? RAYWHITE : Color{35, 55, 53, 255}, 0, 4.0f);
			}
		}

		// Action buttons
		Rectangle saveBtn = {modal.x + 30, modal.y + 415, 220, 42};
		Rectangle cancelBtn = {modal.x + 270, modal.y + 415, 220, 42};
		Anim::Button(saveBtn, Color{35, 105, 78, 255}, mouse, true, 0.2f);
		DrawUiTextInRect("LƯU TÀI KHOẢN", saveBtn, 13, RAYWHITE, 0, 8.0f);
		Anim::Button(cancelBtn, Color{150, 160, 152, 255}, mouse, true, 0.2f);
		DrawUiTextInRect("HUỶ BỎ", cancelBtn, 13, RAYWHITE, 0, 8.0f);
	}
}

void StaffManagementUI::drawChangePassModal(Vector2 mouse, const User& currentUser) {
	if (!isChangePassModalOpen) return;

	DrawRectangle(0, 0, 1280, 760, Color{0, 0, 0, 140});
	Rectangle modal = {400, 180, 480, 370};
	DrawRectangleRounded(modal, 0.05f, 8, RAYWHITE);
	DrawRectangleRoundedLines(modal, 0.05f, 8, Color{35, 105, 78, 255});

	DrawUiTextFitted("ĐỔI MẬT KHẨU TÀI KHOẢN", modal.x + 30, modal.y + 24, 420, 18, Color{35, 55, 53, 255});
	std::string userDesc = "Tài khoản hiện tại: " + currentUser.username + " (" + currentUser.getName() + ")";
	DrawUiTextFitted(userDesc.c_str(), modal.x + 30, modal.y + 48, 420, 12, Color{104, 115, 108, 255});

	// Field 0: Old Password
	DrawUiText("Mật khẩu hiện tại:", modal.x + 30, modal.y + 74, 12, Color{35, 55, 53, 255});
	Rectangle oldBox = {modal.x + 30, modal.y + 94, 420, 36};
	DrawUiInputBox(oldBox, inputOldPassword, "Nhập mật khẩu hiện tại...", changePassField == 0, mouse, true);

	// Field 1: New Password
	DrawUiText("Mật khẩu mới:", modal.x + 30, modal.y + 142, 12, Color{35, 55, 53, 255});
	Rectangle newBox = {modal.x + 30, modal.y + 162, 420, 36};
	DrawUiInputBox(newBox, inputNewPassword, "Nhập mật khẩu mới (ít nhất 4 ký tự)...", changePassField == 1, mouse, true);

	// Field 2: Confirm Password
	DrawUiText("Xác nhận mật khẩu mới:", modal.x + 30, modal.y + 210, 12, Color{35, 55, 53, 255});
	Rectangle confBox = {modal.x + 30, modal.y + 230, 420, 36};
	DrawUiInputBox(confBox, inputConfirmPassword, "Nhập lại mật khẩu mới...", changePassField == 2, mouse, true);

	// Action buttons
	Rectangle saveBtn = {modal.x + 30, modal.y + 295, 200, 42};
	Rectangle cancelBtn = {modal.x + 250, modal.y + 295, 200, 42};
	Anim::Button(saveBtn, Color{35, 105, 78, 255}, mouse, true, 0.2f);
	DrawUiTextInRect("LƯU MẬT KHẨU", saveBtn, 13, RAYWHITE, 0, 8.0f);
	Anim::Button(cancelBtn, Color{150, 160, 152, 255}, mouse, true, 0.2f);
	DrawUiTextInRect("HUỶ BỎ", cancelBtn, 13, RAYWHITE, 0, 8.0f);
}

