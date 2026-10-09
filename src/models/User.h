#pragma once

#include <iostream>
#include <string>

class ScheduleService;

enum class Role { Employee, Manager, BranchManager };

class User {
protected:
	std::string id;
	std::string name;
public:
	std::string username;
	Role role;
	User(std::string userId,std::string userName,Role userRole);
	virtual ~User() = default;
	virtual Role getRole() const = 0;
	virtual void showMenuOptions() const = 0;
	std::string getName() const;
	virtual bool canManageBranches() const;
	virtual bool canManageAllBranches() const;
	virtual bool canViewReports() const;
};

class Employee : public User {
public:
	std::string branchId;
	Employee(std::string id="EMP001",std::string name="Employee",std::string branch="BR001");
	std::string getId() const;
	int getMinimumShifts() const;
	void setMinimumShifts(int minimum);
	bool registerForEmptyShift(ScheduleService& scheduleService, const std::string& scheduleId);
	Role getRole() const override;
	void showMenuOptions() const override;
	bool canViewReports() const override;

private:
	int minimumShifts = 0;
};

class Manager : public Employee {
public:
	Manager(std::string id="MGR001",std::string name="Manager",std::string branch="BR001");
	Role getRole() const override;
	void showMenuOptions() const override;
	bool canManageBranches() const override;
};

class BranchManager : public Manager {
public:
	BranchManager(std::string id="BM001",std::string name="Branch Manager");
	Role getRole() const override;
	void showMenuOptions() const override;
	bool canManageAllBranches() const override;
};
