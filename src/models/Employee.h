#pragma once

#include "../datastructures/String.h"

class ScheduleService;

// Phân cấp 3 vai trò nhân sự trong chuỗi nhà hàng
enum class Role {
	Employee,
	Manager,
	ChainManager
};

// Lớp cơ sở (Cấp 1): Nhân viên (Employee)
class Employee {
protected:
	String id;
	String name;
	String username;
	String branchId;
	Role role;
	int minimumShifts = 0;

public:
	Employee(const String& empId = "EMP001", const String& empName = "Employee", const String& branch = "BR001");
	virtual ~Employee() = default;

	String getId() const;
	String getName() const;
	void setName(const String& value);
	String getUsername() const;
	void setUsername(const String& value);
	virtual String getBranchId() const;
	void setBranchId(const String& value);

	int getMinimumShifts() const;
	void setMinimumShifts(int minimum);
	bool registerForEmptyShift(ScheduleService& scheduleService, const String& scheduleId);

	// Các phương thức ảo thể hiện tính Đa hình (Polymorphism) theo cấp bậc
	virtual Role getRole() const;
	virtual const char* getRoleTitle() const;
	virtual bool canManageBranches() const;
	virtual bool canManageAllBranches() const;
	virtual bool canViewReports() const;
	virtual bool canAccessTab(int tabId) const;

	// Nạp chồng toán tử so sánh
	bool operator==(const Employee& other) const { return id == other.id; }
	bool operator!=(const Employee& other) const { return id != other.id; }
};

// Lớp kế thừa Cấp 2: Quản lý chi nhánh (Manager kế thừa Employee)
class Manager : public Employee {
public:
	Manager(const String& mgrId = "MGR001", const String& mgrName = "Manager", const String& branch = "BR001");
	Role getRole() const override;
	const char* getRoleTitle() const override;
	bool canManageBranches() const override;
	bool canViewReports() const override;
	bool canAccessTab(int tabId) const override;
};

// Lớp kế thừa Cấp 3: Giám đốc chuỗi / Tổng quản lý (ChainManager kế thừa Manager)
class ChainManager : public Manager {
public:
	ChainManager(const String& cmId = "CM001", const String& cmName = "Chain Manager");
	Role getRole() const override;
	const char* getRoleTitle() const override;
	bool canManageAllBranches() const override;
	bool canAccessTab(int tabId) const override;
};

