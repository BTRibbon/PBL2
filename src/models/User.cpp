#include "User.h"

User::User(std::string userId,std::string userName,Role userRole) : id(userId),name(userName),username(userName),role(userRole) {}

std::string User::getName() const { return name; }

bool User::canManageBranches() const { return false; }

bool User::canViewReports() const { return false; }

Customer::Customer(std::string id,std::string name) : User(id,name,Role::Customer) {}

Role Customer::getRole() const { return Role::Customer; }

void Customer::showMenuOptions() const { std::cout<<"[Customer] Xem menu / Dat mon / Xem gio hang\n"; }

Employee::Employee(std::string id,std::string name,std::string branch) : User(id,name,Role::Employee),branchId(branch) {}

Role Employee::getRole() const { return Role::Employee; }

void Employee::showMenuOptions() const { std::cout<<"[Employee] Xu ly don hang / Cap nhat trang thai order\n"; }

bool Employee::canViewReports() const { return true; }

Manager::Manager(std::string id,std::string name,std::string branch) : Employee(id,name,branch) { role=Role::Manager; }

Role Manager::getRole() const { return Role::Manager; }

void Manager::showMenuOptions() const { std::cout<<"[Manager] Quan ly chi nhanh / Xem bao cao / Quan ly nhan vien\n"; }

bool Manager::canManageBranches() const { return true; }
