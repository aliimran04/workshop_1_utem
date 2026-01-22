#include "ReportGeneration.h"
#include "utils.h" // Assuming readInt is defined here
#include <iostream>
#include <iomanip>
#include <string>
#include <memory>
#include <vector>
#include <cmath>

using namespace std;

void runReportGeneration(sql::Connection* con) {
    int choice;
    do {
        cout << "\n=====================================";
        cout << "\n      Report Generation Module       ";
        cout << "\n=====================================";
        cout << "\n1. Financial Summary (Summary List)";
        cout << "\n2. Sales Trend (Text Bar Chart)";
        cout << "\n3. Sales Growth (Graph Summary %)";
        cout << "\n4. Monthly Sales (Table Format)";
        cout << "\n5. Exit to Main Menu";
        cout << "\n=====================================";
        cout << "\nEnter choice: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(1000, '\n');
            continue;
        }

        switch (choice) {
        case 1: { // Financial Summary
            int y = readInt("Enter Year (e.g., 2024): ");
            int m = readInt("Enter Month (1-12): ");
            if (m >= 1 && m <= 12) {
                generateFinancialSummary(con, y, m);
            }
            else {
                cout << "Invalid month!\n";
            }
            break;
        }
        case 2: { // Sales Trend
            int y = readInt("Enter Year for Trend Analysis : ");
            displaySalesTrendChart(con, y);
            break;
        }
        case 3: { // Sales Growth
            int y = readInt("Enter Year for Growth Analysis : ");
            displaySalesGrowthGraph(con, y);
            break;
        }
        case 4: { // Monthly Sales Table
            int y = readInt("Enter Year (e.g., 2025): ");
            int m = readInt("Enter Month (1-12): ");

            if (m < 1 || m > 12) {
                cout << "[Error] Month must be between 1 and 12.\n";
            }
            else {
                displayMonthlySalesTable(con, y, m);
            }
            break;
        }
        case 5:
            cout << "Returning to Main Menu...\n";
            break;
        default:
            cout << "Invalid option!\n";
        }
    } while (choice != 5);
}

// 1. FINANCIAL SUMMARY
void generateFinancialSummary(sql::Connection* con, int year, int month) {
    try {
        unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT "
                "  (SELECT IFNULL(SUM(Amount), 0) FROM payment "
                "   WHERE PaymentStatus = 'Complete' AND YEAR(TimeStamp) = ? AND MONTH(TimeStamp) = ?) AS TotalSales, "
                "  (SELECT IFNULL(SUM(Quantity * UnitCost), 0) FROM inventory) AS TotalAssets, "
                "  (SELECT IFNULL(SUM(ic.QuantityUsed * i.UnitCost), 0) "
                "   FROM inventoryconsumption ic "
                "   JOIN inventory i ON ic.InventoryID = i.InventoryID "
                "   WHERE YEAR(ic.TimeStamp) = ? AND MONTH(ic.TimeStamp) = ?) AS TotalCost"
            )
        );

        pstmt->setInt(1, year);
        pstmt->setInt(2, month);
        pstmt->setInt(3, year);
        pstmt->setInt(4, month);

        unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            double sales = res->getDouble("TotalSales");
            double assetVal = res->getDouble("TotalAssets");
            double cost = res->getDouble("TotalCost");
            double profit = sales - cost;
            double margin = (sales > 0) ? (profit / sales) * 100 : 0.0;

            cout << "\n==========================================" << endl;
            cout << "    FINANCIAL SUMMARY FOR " << month << "/" << year << endl;
            cout << "==========================================" << endl;
            cout << left << setw(28) << "1. Monthly Total Sales:" << "$" << fixed << setprecision(2) << sales << endl;
            cout << left << setw(28) << "2. Total Operating Cost:" << "$" << cost << endl;
            cout << "------------------------------------------" << endl;
            cout << left << setw(28) << "3. Net Profit:" << "$" << profit << endl;
            cout << left << setw(28) << "4. Profit Margin:" << margin << "%" << endl;
            cout << "------------------------------------------" << endl;
            cout << left << setw(28) << "5. Total Current Assets:" << "$" << assetVal << " (Current)" << endl;
            cout << "==========================================" << endl;

            if (sales == 0 && cost == 0) {
                cout << "[Notice] No data found for this specific period." << endl;
            }
        }
    }
    catch (sql::SQLException& e) {
        cerr << "SQL Error in Financial Summary: " << e.what() << endl;
    }
}

// 2. SALES TREND
void displaySalesTrendChart(sql::Connection* con, int year) {
    try {
        unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT MONTHNAME(TimeStamp) AS Month, SUM(Amount) AS MonthlySales "
                "FROM payment "
                "WHERE PaymentStatus = 'Complete' AND YEAR(TimeStamp) = ? "
                "GROUP BY MONTH(TimeStamp), MONTHNAME(TimeStamp) "
                "ORDER BY MONTH(TimeStamp) ASC"
            )
        );
        pstmt->setInt(1, year);
        unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        std::cout << "\n--- Sales Trend for " << year << " (Scale: 1 # = $500) ---\n";
        while (res->next()) {
            string month = res->getString("Month");
            double sales = res->getDouble("MonthlySales");
            // Change from / 100 to / 5000 or / 10000
            int barWidth = static_cast<int>(sales / 500);

            

            cout << left << setw(12) << month << " | ";
            for (int i = 0; i < barWidth; ++i) cout << "#";
            cout << "  $" << fixed << setprecision(2) << sales << endl;
        }
    }
    catch (sql::SQLException& e) { cerr << "SQL Error: " << e.what() << endl; }
}

// 3. SALES GROWTH
void displaySalesGrowthGraph(sql::Connection* con, int year) {
    try {
        unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT Month, MonthlySales, PrevSales FROM ("
                "  SELECT MONTHNAME(TimeStamp) AS Month, "
                "  YEAR(TimeStamp) as YearVal, "
                "  MONTH(TimeStamp) as MonthNum, "
                "  SUM(Amount) AS MonthlySales, "
                "  LAG(SUM(Amount)) OVER (ORDER BY YEAR(TimeStamp), MONTH(TimeStamp)) AS PrevSales "
                "  FROM payment WHERE PaymentStatus = 'Complete' "
                "  GROUP BY YEAR(TimeStamp), MONTH(TimeStamp), MONTHNAME(TimeStamp)"
                ") AS GrowthData WHERE YearVal = ?"
            )
        );
        pstmt->setInt(1, year);
        unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        cout << "\n--- Monthly Sales Growth Graph for " << year << " ---\n";
        while (res->next()) {
            string month = res->getString("Month");
            double current = res->getDouble("MonthlySales");
            double previous = res->getDouble("PrevSales");

            cout << left << setw(12) << month << ": ";
            if (previous <= 0) {
                cout << "[No Previous Data]";
            }
            else {
                double growth = ((current - previous) / previous) * 100;
                cout << (growth >= 0 ? "+" : "") << fixed << setprecision(1) << growth << "% ";
                int blocks = static_cast<int>(abs(growth) / 10);
                char marker = (growth >= 0 ? '+' : '-');
                for (int i = 0; i < blocks; i++) cout << marker;
            }
            cout << endl;
        }
    }
    catch (sql::SQLException& e) { cerr << "SQL Error: " << e.what() << endl; }
}

// 4. MONTHLY SALES DATA
// 4. MONTHLY SALES DATA (Optimized for Big Data)
void displayMonthlySalesTable(sql::Connection* con, int year, int month) {
    try {
        // Step 1: Summary Query
        unique_ptr<sql::PreparedStatement> summaryPstmt(
            con->prepareStatement(
                "SELECT COUNT(*) AS total_count, IFNULL(SUM(Amount), 0) AS total_sum "
                "FROM payment WHERE PaymentStatus = 'Complete' "
                "AND YEAR(TimeStamp) = ? AND MONTH(TimeStamp) = ?"
            )
        );
        summaryPstmt->setInt(1, year);
        summaryPstmt->setInt(2, month);
        unique_ptr<sql::ResultSet> summaryRes(summaryPstmt->executeQuery());

        long long totalRows = 0;
        double totalRevenue = 0;
        if (summaryRes->next()) {
            totalRows = summaryRes->getInt64("total_count");
            totalRevenue = summaryRes->getDouble("total_sum");
        }

        if (totalRows == 0) {
            cout << "\n[Notice] No transactions found for " << month << "/" << year << ".\n";
            return;
        }

        cout << "\n==================================================================" << endl;
        cout << "   SUMMARY FOR " << month << "/" << year << endl;
        cout << "   Total Transactions: " << totalRows << endl;
        cout << "   Total Revenue:      $" << fixed << setprecision(2) << totalRevenue << endl;
        cout << "==================================================================" << endl;
        cout << "Proceed to view detailed list? (y/n): ";
        char proceed; cin >> proceed;
        if (tolower(proceed) != 'y') return;

        // Step 2: Detailed List Query
        unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT p.TransactionID, u.FullName, p.Amount, p.TimeStamp "
                "FROM payment p JOIN user u ON p.UserID = u.UserID "
                "WHERE p.PaymentStatus = 'Complete' AND YEAR(p.TimeStamp) = ? AND MONTH(p.TimeStamp) = ? "
                "ORDER BY p.TimeStamp ASC"
            )
        );
        pstmt->setInt(1, year);
        pstmt->setInt(2, month);
        unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        int rowCount = 0;
        int pageSize = 20;

        // Header printing logic in a lambda to avoid repetition
        auto printHeader = [&]() {
            cout << "==================================================================" << endl;
            cout << "| " << left << setw(10) << "ID"
                << "| " << setw(22) << "Customer Name"
                << "| " << setw(12) << "Amount"
                << "| " << setw(12) << "Date" << " |" << endl;
            cout << "------------------------------------------------------------------" << endl;
            };

#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif

        printHeader();

        while (res->next()) {
            cout << "| " << left << setw(10) << res->getInt("TransactionID")
                << "| " << setw(22) << res->getString("FullName")
                << "| $" << setw(11) << fixed << setprecision(2) << res->getDouble("Amount")
                << "| " << res->getString("TimeStamp").substr(0, 10) << " |" << endl;

            rowCount++;

            if (rowCount % pageSize == 0) {
                cout << "------------------------------------------------------------------" << endl;
                cout << ">>> Showing " << rowCount << " of " << totalRows << " records." << endl;
                cout << ">>> [Enter] for more, [q] to quit, [c] to clear & continue: ";

                string input;
                getline(cin >> ws, input);

                if (!input.empty() && tolower(input[0]) == 'q') break;

                if (!input.empty() && tolower(input[0]) == 'c') {
#ifdef _WIN32
                    system("cls");
#else
                    system("clear");
#endif
                    printHeader(); // Use the lambda to reprint headers
                }
            }
        }
        cout << "========================== END OF REPORT ==========================" << endl;

    }
    catch (sql::SQLException& e) {
        cerr << "SQL Error: " << e.what() << endl;
    }
}