#pragma once

#include "raylib.h"
#include "../models/AccountRecord.h"
#include "../models/Branch.h"
#include "../models/User.h"
#include <string>
#include <vector>
#include <memory>

class StaffManagementUI {
public:
	// Filter state
	int selectedBranchFilter = -1; // -1 = All branches
	int selectedRoleFilter = -1;   // -1 = All, 0 = Manager, 1 = Employee
	int tableScrollOffset = 0;

	// Add / Edit Staff Modal
	bool isStaffModalOpen = false;
	int staffModalMode = 0; // 0 = Add, 1 = Edit
	int editingAccountIdx = -1;
	std::string inputStaffId;
	std::string inputStaffName;
	std::string inputStaffUsername;
	std::string inputStaffPassword;
	std::string inputStaffBranchId;
	std::string inputStaffRole; // "Employee" or "Manager"
	int staffModalField = 0;

	// Change Password Modal (available for all users)
	bool isChangePassModalOpen = false;
	std::string inputOldPassword;
	std::string inputNewPassword;
	std::string inputConfirmPassword;
	int changePassField = 0;

	StaffManagementUI();

	void openAddStaffModal(const User& currentUser, const std::vector<Branch>& branches, const std::vector<AccountRecord>& accounts);
	void openEditStaffModal(int accountIdx, const std::vector<AccountRecord>& accounts);
	void openChangePassModal();

	bool submitStaffModal(std::vector<AccountRecord>& accounts, std::vector<std::unique_ptr<User>>& userPool,
		const User& currentUser, std::string& statusMessage, int& statusVersion);
	bool submitChangePassModal(std::vector<AccountRecord>& accounts, User& currentUser,
		std::string& statusMessage, int& statusVersion);

	void deleteStaff(int accountIdx, std::vector<AccountRecord>& accounts,
		const User& currentUser, std::string& statusMessage, int& statusVersion);

	bool handleClick(Vector2 mouse, std::vector<AccountRecord>& accounts,
		std::vector<std::unique_ptr<User>>& userPool, const std::vector<Branch>& branches,
		User& currentUser, std::string& statusMessage, int& statusVersion);

	void handleScroll(Vector2 mouse, float wheelMove);
	bool handleInput(int key, int charPressed, std::vector<AccountRecord>& accounts,
		std::vector<std::unique_ptr<User>>& userPool, User& currentUser,
		std::string& statusMessage, int& statusVersion);

	void draw(Vector2 mouse, const std::vector<AccountRecord>& accounts,
		const std::vector<Branch>& branches, User& currentUser);
	void drawChangePassModal(Vector2 mouse, const User& currentUser);

	bool isModalOpen() const { return isStaffModalOpen || isChangePassModalOpen; }
};

