#pragma once

#include "raylib.h"
#include "../datastructures/String.h"
#include "../datastructures/Vector.h"
#include "../models/AccountRecord.h"
#include "../models/Branch.h"
#include "../models/Employee.h"

class StaffManagementUI {
public:
	int selectedBranchFilter = -1; // -1 = Tất cả chi nhánh
	int selectedRoleFilter = -1;
	int tableScrollOffset = 0;

	// Hộp thoại Thêm / Sửa nhân sự
	bool isStaffModalOpen = false;
	int staffModalMode = 0; // 0 = Thêm, 1 = Sửa
	int editingAccountIdx = -1;
	String inputStaffId;
	String inputStaffName;
	String inputStaffUsername;
	String inputStaffPassword;
	String inputStaffBranchId;
	String inputStaffRole; // "Employee" hoặc "Manager"
	int staffModalField = 0;

	// Hộp thoại Đổi mật khẩu (dành cho mọi cấp bậc)
	bool isChangePassModalOpen = false;
	String inputOldPassword;
	String inputNewPassword;
	String inputConfirmPassword;
	int changePassField = 0;

	StaffManagementUI();

	void openAddStaffModal(const Employee& currentUser, const Vector<Branch>& branches, const Vector<AccountRecord>& accounts);
	void openEditStaffModal(int accountIdx, const Vector<AccountRecord>& accounts);
	void openChangePassModal();

	bool submitStaffModal(Vector<AccountRecord>& accounts, const Employee& currentUser,
		String& statusMessage, int& statusVersion);
	bool submitChangePassModal(Vector<AccountRecord>& accounts, Employee& currentUser,
		String& statusMessage, int& statusVersion);

	void deleteStaff(int accountIdx, Vector<AccountRecord>& accounts,
		const Employee& currentUser, String& statusMessage, int& statusVersion);

	bool handleClick(Vector2 mouse, Vector<AccountRecord>& accounts,
		const Vector<Branch>& branches, Employee& currentUser,
		String& statusMessage, int& statusVersion);

	void handleScroll(Vector2 mouse, float wheelMove);
	bool handleInput(int key, int charPressed, Vector<AccountRecord>& accounts,
		Employee& currentUser, String& statusMessage, int& statusVersion);

	void draw(Vector2 mouse, const Vector<AccountRecord>& accounts,
		const Vector<Branch>& branches, Employee& currentUser);
	void drawChangePassModal(Vector2 mouse, const Employee& currentUser);

	bool isModalOpen() const { return isStaffModalOpen || isChangePassModalOpen; }
};
