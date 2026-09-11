#pragma once

#include <iostream>
#include <string>

enum class Role { Customer, Employee, Manager };

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
	virtual bool canViewReports() const;
};

class Customer : public User {
public:
	Customer(std::string id="CUS001",std::string name="Kiosk Guest");
	Role getRole() const override;
	void showMenuOptions() const override;
};

class Employee : public User {
public:
	std::string branchId;
	Employee(std::string id="EMP001",std::string name="Employee",std::string branch="BR001");
	Role getRole() const override;
	void showMenuOptions() const override;
	bool canViewReports() const override;
};

class Manager : public Employee {
public:
	Manager(std::string id="MGR001",std::string name="Manager",std::string branch="BR001");
	Role getRole() const override;
	void showMenuOptions() const override;
	bool canManageBranches() const override;
};
