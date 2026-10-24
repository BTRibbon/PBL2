#include "ReportUI.h"
#include "UIAnim.h"
#include "UIFont.h"
#include "../services/DataManager.h"
#include <cmath>

ReportStats ReportUI::buildReportStats(const Vector<Order>& orders, const Employee& user,
	const Vector<Branch>& branches) const {
	ReportStats stats;
	const bool isChainManager = user.canManageAllBranches();

	String filteredBranchName = "";
	if (!isChainManager) {
		for (int i = 0; i < branches.size(); ++i) {
			if (branches[i].getId() == user.getBranchId()) {
				filteredBranchName = branches[i].getName();
				break;
			}
		}
	} else if (selectedReportBranch_ >= 0 && selectedReportBranch_ < branches.size()) {
		filteredBranchName = branches[selectedReportBranch_].getName();
	}

	for (int i = 0; i < branches.size(); ++i) {
		stats.branchBreakdown.push_back({branches[i].getName(), branches[i].getId(), 0, 0, 0});
	}

	for (int o = 0; o < orders.size(); ++o) {
		const Order& order = orders[o];
		for (int bsIdx = 0; bsIdx < stats.branchBreakdown.size(); ++bsIdx) {
			BranchStat& bs = stats.branchBreakdown[bsIdx];
			for (int bIdx = 0; bIdx < branches.size(); ++bIdx) {
				if (branches[bIdx].getId() == bs.branchId && DataManager::matchBranch(order.branch, branches[bIdx])) {
					bs.revenue += order.total;
					bs.totalOrders++;
					if (order.status == "pending") bs.pendingOrders++;
					break;
				}
			}
		}

		if (!filteredBranchName.empty()) {
			bool matches = false;
			for (int bIdx = 0; bIdx < branches.size(); ++bIdx) {
				if (branches[bIdx].getName() == filteredBranchName && DataManager::matchBranch(order.branch, branches[bIdx])) {
					matches = true;
					break;
				}
			}
			if (!matches) continue;
		}

		stats.revenue += order.total;
		stats.orderCount++;
		if (order.status == "pending") stats.pendingCount++;

		for (int it = 0; it < order.items.size(); ++it) {
			stats.addQuantity(order.items[it].productCode, order.items[it].quantity);
		}
	}

	return stats;
}

void ReportUI::handleScroll(Vector2 mouse, float wheel, const Vector<Branch>& branches,
	const Vector<Product>& products, const Employee& currentUser) {
	if (std::fabs(wheel) <= 0.01f) return;
	int step = (wheel > 0) ? -1 : 1;
	const bool isChainManager = currentUser.canManageAllBranches();

	if (CheckCollisionPointRec(mouse, {245, 134, 1000, 36})) {
		int maxBrFilter = maxVal(0, branches.size() - 3);
		reportBranchFilterScroll_ = clampVal(reportBranchFilterScroll_ + step, 0, maxBrFilter);
	} else if (isChainManager && CheckCollisionPointRec(mouse, {245, 290, 1000, 195})) {
		int maxBrTable = maxVal(0, branches.size() - 4);
		reportBranchTableScroll_ = clampVal(reportBranchTableScroll_ + step, 0, maxBrTable);
	} else {
		int visProds = isChainManager ? 4 : 7;
		int maxProdScroll = maxVal(0, products.size() - visProds);
		reportProductScroll_ = clampVal(reportProductScroll_ + step, 0, maxProdScroll);
	}
}

void ReportUI::handleClick(Vector2 mouse, Vector<Order>& orders, OrderQueue& orderQueue,
	const Vector<Branch>& branches, const Employee& currentUser,
	String& statusMessage, int& statusVersion) {
	// Nút xử lý đơn hàng chờ tiếp theo trong OrderQueue ngay trên thẻ Đơn Chờ Xử Lý
	if (!orderQueue.empty() && CheckCollisionPointRec(mouse, {1007, 176, 238, 84})) {
		Order nextOrd = orderQueue.popNext();
		for (int i = 0; i < orders.size(); ++i) {
			if (orders[i].id == nextOrd.id) {
				orders[i].status = "completed";
				break;
			}
		}
		DataManager::saveOrders(orders);
		statusVersion++;
		statusMessage = String("Đã hoàn tất xử lý đơn chờ #") + nextOrd.orderId + " (" + nextOrd.branch + ")";
		return;
	}

	if (currentUser.canManageAllBranches()) {
		int totalBr = branches.size();
		const int visBrPills = 3;
		int maxBrFilter = maxVal(0, totalBr - visBrPills);
		reportBranchFilterScroll_ = clampVal(reportBranchFilterScroll_, 0, maxBrFilter);

		if (CheckCollisionPointRec(mouse, {310, 138, 110, 28})) {
			selectedReportBranch_ = -1;
		}
		if (CheckCollisionPointRec(mouse, {428, 138, 30, 28})) {
			reportBranchFilterScroll_ = maxVal(0, reportBranchFilterScroll_ - 1);
		}
		int shown = minVal(totalBr - reportBranchFilterScroll_, visBrPills);
		for (int b = 0; b < shown; ++b) {
			int brIdx = reportBranchFilterScroll_ + b;
			Rectangle bBtn = {466.0f + b * 244.0f, 138.0f, 236.0f, 28.0f};
			if (CheckCollisionPointRec(mouse, bBtn)) {
				selectedReportBranch_ = brIdx;
			}
		}
		if (CheckCollisionPointRec(mouse, {1202, 138, 30, 28})) {
			reportBranchFilterScroll_ = minVal(maxBrFilter, reportBranchFilterScroll_ + 1);
		}
	}
}

void ReportUI::draw(Vector2 uiMouse, const Vector<Order>& orders, const OrderQueue& orderQueue,
	const Vector<Branch>& branches, const Vector<Product>& products, const Employee& currentUser) {
	const Color green{35, 105, 78, 255};
	const Color darkGreen{35, 55, 53, 255};
	const Color amber{180, 110, 40, 255};
	const Color muted{90, 100, 94, 255};
	const Color panel = RAYWHITE;
	const Color panelAlt{248, 251, 248, 255};

	const bool isChainManager = currentUser.canManageAllBranches();
	const ReportStats report = buildReportStats(orders, currentUser, branches);
	const int averageOrder = report.orderCount == 0 ? 0 : report.revenue / report.orderCount;

	DrawUiText("Báo cáo Doanh thu & Bán hàng", 245, 104, 24, darkGreen);

	if (isChainManager) {
		DrawUiText("Bộ lọc:", 245, 144, 13, {90, 100, 94, 255});
		Rectangle allBtn = {310, 138, 110, 28};
		bool allActive = selectedReportBranch_ == -1;
		Color allBase = allActive ? green : Color{235, 240, 235, 255};
		Color allHov = allActive ? BrightenColor(green, 15) : Color{220, 232, 222, 255};
		Rectangle allV = Anim::Button(allBtn, allBase, allHov, uiMouse, true, 0.2f, !allActive);
		DrawUiTextInRect("Tất cả CN", allV, 12, allActive ? RAYWHITE : darkGreen, 0, 4.0f);

		int totalBr = branches.size();
		const int visBrPills = 3;
		int maxBrFilter = maxVal(0, totalBr - visBrPills);
		reportBranchFilterScroll_ = clampVal(reportBranchFilterScroll_, 0, maxBrFilter);

		Rectangle leftBrBtn = {428, 138, 30, 28};
		Anim::ArrowButton(leftBrBtn, Anim::ArrowDir::Left, uiMouse, reportBranchFilterScroll_ > 0, darkGreen);

		int shown = minVal(totalBr - reportBranchFilterScroll_, visBrPills);
		for (int b = 0; b < shown; ++b) {
			int brIdx = reportBranchFilterScroll_ + b;
			Rectangle bBtn = {466.0f + b * 244.0f, 138.0f, 236.0f, 28.0f};
			bool bActive = selectedReportBranch_ == brIdx;
			Color bBase = bActive ? green : Color{235, 240, 235, 255};
			Color bHov = bActive ? BrightenColor(green, 15) : Color{220, 232, 222, 255};
			Rectangle bV = Anim::Button(bBtn, bBase, bHov, uiMouse, true, 0.2f, !bActive);
			DrawUiTextInRect(branches[brIdx].getName().c_str(), bV, 12, bActive ? RAYWHITE : darkGreen, 0, 6.0f);
		}

		Rectangle rightBrBtn = {1202, 138, 30, 28};
		Anim::ArrowButton(rightBrBtn, Anim::ArrowDir::Right, uiMouse, reportBranchFilterScroll_ < maxBrFilter, darkGreen);
	} else {
		String subTitle = String("Phạm vi: ") + currentUser.getBranchId() + " (Chi nhánh hiện tại của bạn)";
		DrawUiTextFitted(subTitle.c_str(), 245, 142, 980, 13, muted);
	}

	// 4 Summary Metric Cards
	const Rectangle cards[4] = {
		{245, 176, 238, 84}, {499, 176, 238, 84}, {753, 176, 238, 84}, {1007, 176, 238, 84}
	};
	const Color cardColors[4] = {
		{35, 105, 78, 255}, {180, 110, 40, 255}, {60, 110, 130, 255}, {120, 85, 130, 255}
	};
	const char* cardLabels[4] = {"TỔNG DOANH THU", "SỐ ĐƠN HÀNG", "TRUNG BÌNH / ĐƠN", "ĐƠN CHỜ XỬ LÝ"};
	const String cardValues[4] = {
		DataManager::formatCurrency(report.revenue),
		String::fromInt(report.orderCount) + " đơn",
		DataManager::formatCurrency(averageOrder),
		String::fromInt(report.pendingCount) + " đơn" + (!orderQueue.empty() ? " (Nhấp xử lý)" : "")
	};

	for (int i = 0; i < 4; ++i) {
		if (i == 3 && !orderQueue.empty()) {
			Rectangle cardV = Anim::Button(cards[i], cardColors[i], BrightenColor(cardColors[i], 18), uiMouse, true, 0.15f);
			DrawUiTextFitted(cardLabels[i], cardV.x + 16, cardV.y + 14, 206, 12, {235, 242, 235, 255});
			DrawUiTextFitted(cardValues[i].c_str(), cardV.x + 16, cardV.y + 42, 206, 16, RAYWHITE);
		} else {
			DrawRectangleRounded(cards[i], 0.15f, 4, cardColors[i]);
			DrawUiTextFitted(cardLabels[i], cards[i].x + 16, cards[i].y + 14, 206, 12, {235, 242, 235, 255});
			DrawUiTextFitted(cardValues[i].c_str(), cards[i].x + 16, cards[i].y + 42, 206, 19, RAYWHITE);
		}
	}

	if (isChainManager) {
		DrawUiText("DOANH THU CHI TIẾT THEO TỪNG CHI NHÁNH", 245, 274, 14, darkGreen);

		int tableY = 298;
		DrawRectangle(245, tableY, 985, 32, Color{232, 238, 233, 255});
		DrawUiText("CHI NHÁNH", 265, tableY + 8, 12, darkGreen);
		DrawUiText("MÃ CN", 525, tableY + 8, 12, darkGreen);
		DrawUiText("TỔNG ĐƠN", 635, tableY + 8, 12, darkGreen);
		DrawUiText("ĐƠN CHỜ", 755, tableY + 8, 12, darkGreen);
		DrawUiText("DOANH THU", 875, tableY + 8, 12, darkGreen);
		DrawUiText("TỶ TRỌNG", 1085, tableY + 8, 12, darkGreen);

		int rowY = tableY + 34;
		int totalChainRevenue = 0;
		for (int i = 0; i < report.branchBreakdown.size(); ++i) {
			totalChainRevenue += report.branchBreakdown[i].revenue;
		}

		const int visBrRows = 4;
		int totalBrRows = report.branchBreakdown.size();
		reportBranchTableScroll_ = clampVal(reportBranchTableScroll_, 0, maxVal(0, totalBrRows - visBrRows));

		for (int r = 0; r < visBrRows; ++r) {
			int b = reportBranchTableScroll_ + r;
			if (b >= totalBrRows) break;
			const BranchStat& bs = report.branchBreakdown[b];
			DrawRectangle(245, rowY, 985, 36, (b % 2 == 0) ? panel : panelAlt);
			DrawRectangleLines(245, rowY, 985, 36, Color{230, 235, 230, 255});

			DrawUiTextFitted(bs.branchName.c_str(), 265, rowY + 10, 245, 13, darkGreen);
			DrawUiTextFitted(bs.branchId.c_str(), 525, rowY + 10, 95, 12, muted);
			DrawUiTextFitted((String::fromInt(bs.totalOrders) + " đơn").c_str(), 635, rowY + 10, 105, 13, darkGreen);
			DrawUiTextFitted((String::fromInt(bs.pendingOrders) + " đơn").c_str(), 755, rowY + 10, 105, 13, bs.pendingOrders > 0 ? amber : green);
			DrawUiTextFitted(DataManager::formatCurrency(bs.revenue).c_str(), 875, rowY + 10, 190, 13, green);

			int percent = totalChainRevenue > 0 ? (bs.revenue * 100 / totalChainRevenue) : 0;
			DrawUiTextFitted((String::fromInt(percent) + " %").c_str(), 1085, rowY + 10, 120, 13, darkGreen);

			rowY += 38;
		}
		DrawVerticalScrollbar({1236, static_cast<float>(tableY + 34), 8, static_cast<float>(visBrRows * 38 - 2)},
			totalBrRows, visBrRows, reportBranchTableScroll_);

		DrawUiText("DANH SÁCH MÓN ĂN ĐÃ BÁN (Lăn chuột để cuộn)", 245, 494, 14, darkGreen);
		int pY = 520;
		const int visProdRows = 4;
		int totalProds = products.size();
		reportProductScroll_ = clampVal(reportProductScroll_, 0, maxVal(0, totalProds - visProdRows));

		for (int r = 0; r < visProdRows; ++r) {
			int idx = reportProductScroll_ + r;
			if (idx >= totalProds) break;
			const Product& prod = products[idx];
			int qty = report.getQuantity(prod.code);
			DrawRectangleRounded({245, static_cast<float>(pY), 985, 36}, 0.1f, 4, panel);
			DrawRectangleRoundedLines({245, static_cast<float>(pY), 985, 36}, 0.1f, 4, Color{230, 235, 230, 255});

			DrawUiTextFitted((String::fromInt(idx + 1) + ". " + prod.name + " (" + prod.code + ")").c_str(), 265, pY + 10, 460, 13, darkGreen);
			DrawUiTextFitted((String::fromInt(qty) + " phần đã bán").c_str(), 755, pY + 10, 200, 13, muted);
			DrawUiTextFitted(DataManager::formatCurrency(qty * prod.price).c_str(), 1015, pY + 10, 200, 13, green);

			pY += 42;
		}
		DrawVerticalScrollbar({1236, 520, 8, static_cast<float>(visProdRows * 42 - 6)},
			totalProds, visProdRows, reportProductScroll_);
	} else {
		DrawUiText("SẢN PHẨM BÁN CHẠY TẠI CHI NHÁNH (Lăn chuột để cuộn)", 245, 286, 14, darkGreen);
		int pY = 316;
		const int visProdRows = 7;
		int totalProds = products.size();
		reportProductScroll_ = clampVal(reportProductScroll_, 0, maxVal(0, totalProds - visProdRows));

		for (int r = 0; r < visProdRows; ++r) {
			int idx = reportProductScroll_ + r;
			if (idx >= totalProds) break;
			const Product& prod = products[idx];
			int qty = report.getQuantity(prod.code);
			DrawRectangleRounded({245, static_cast<float>(pY), 985, 42}, 0.1f, 4, panel);
			DrawRectangleRoundedLines({245, static_cast<float>(pY), 985, 42}, 0.1f, 4, Color{230, 235, 230, 255});

			DrawUiTextFitted((String::fromInt(idx + 1) + ". " + prod.name + " (" + prod.code + ")").c_str(), 265, pY + 12, 340, 14, darkGreen);
			DrawUiTextFitted(prod.category.c_str(), 625, pY + 12, 160, 13, muted);
			DrawUiTextFitted((String::fromInt(qty) + " phần").c_str(), 810, pY + 12, 150, 13, darkGreen);
			DrawUiTextFitted(DataManager::formatCurrency(qty * prod.price).c_str(), 990, pY + 12, 220, 14, green);

			pY += 48;
		}
		DrawVerticalScrollbar({1236, 316, 8, static_cast<float>(visProdRows * 48 - 6)},
			totalProds, visProdRows, reportProductScroll_);
	}
}

