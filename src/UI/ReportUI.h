#pragma once
#include "raylib.h"
#include "../datastructures/String.h"
#include "../datastructures/Vector.h"
#include "../datastructures/OrderQueue.h"
#include "../models/Branch.h"
#include "../models/Employee.h"
#include "../models/Order.h"
#include "../services/DataManager.h"

struct BranchStat {
	String branchName;
	String branchId;
	int revenue = 0;
	int totalOrders = 0;
	int pendingOrders = 0;
};

struct ProductSaleCount {
	String code;
	int quantity = 0;
};

struct ReportStats {
	int revenue = 0;
	int orderCount = 0;
	int pendingCount = 0;
	Vector<ProductSaleCount> productQuantities;
	Vector<BranchStat> branchBreakdown;

	int getQuantity(const String& code) const {
		for (int i = 0; i < productQuantities.size(); ++i) {
			if (productQuantities[i].code == code) return productQuantities[i].quantity;
		}
		return 0;
	}

	void addQuantity(const String& code, int qty) {
		for (int i = 0; i < productQuantities.size(); ++i) {
			if (productQuantities[i].code == code) {
				productQuantities[i].quantity += qty;
				return;
			}
		}
		productQuantities.push_back({code, qty});
	}
};

// Giao diện Tab 1: Báo cáo Doanh thu & Thống kê Bán hàng
class ReportUI {
private:
	int selectedReportBranch_ = -1;
	int reportBranchFilterScroll_ = 0;
	int reportBranchTableScroll_ = 0;
	int reportProductScroll_ = 0;

	ReportStats buildReportStats(const Vector<Order>& orders, const Employee& user,
		const Vector<Branch>& branches) const;

public:
	ReportUI() = default;

	void handleScroll(Vector2 mouse, float wheel, const Vector<Branch>& branches,
		const Vector<Product>& products, const Employee& currentUser);

	void handleClick(Vector2 mouse, Vector<Order>& orders, OrderQueue& orderQueue,
		const Vector<Branch>& branches, const Employee& currentUser,
		String& statusMessage, int& statusVersion);

	void draw(Vector2 uiMouse, const Vector<Order>& orders, const OrderQueue& orderQueue,
		const Vector<Branch>& branches, const Vector<Product>& products, const Employee& currentUser);
};
