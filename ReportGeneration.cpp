#include "ReportGeneration.h"
#include "utils.h" // Assumes readInt, clearScreen, etc.
#include <iostream>
#include <iomanip>
#include <limits>
#include <memory>
#include <string>

using namespace std;
using namespace sql;

// ==========================================
// HELPER FUNCTIONS
// ==========================================

// Node R2: Month in 1..12?
bool isValidMonth(int month) {
    return month >= 1 && month <= 12;
}

// ==========================================
// 1) GENERATE MONTHLY REVENUE
// ==========================================
void generateMonthlyRevenue(sql::Connection* con) {
    cout << "\n--- Generate Monthly Revenue ---\n";
    int year, month;

    // Node R1: Get Year and Month
    year = readInt("Enter Year (e.g., 2025): ");
    month = readInt("Enter Month (1-12): ");

    // Node R2: Month in 1..12?
    if (!isValidMonth(month)) {
        cout << "[Error] Invalid Month. Month must be between 1 and 12.\n"; // Node RX0
        return;
    }

    // Node R3: Fetch payments and Node R4: Compute MonthlyRevenue
    double monthlyRevenue = 0.0;

    try {
        // SQL: SUM(Amount) WHERE PaymentStatus='Complete' AND YEAR(PaymentDate)=? AND MONTH(PaymentDate)=?
        // Note: Assumes PaymentDate for the timestamp column.
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT SUM(Amount) AS MonthlyRevenue "
                "FROM payment "
                "WHERE PaymentStatus = 'Complete' AND YEAR(TimeStamp) = ? AND MONTH(TimeStamp) = ?"
            )
        );
        pstmt->setInt(1, year);
        pstmt->setInt(2, month);

        unique_ptr<ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            monthlyRevenue = res->getDouble("MonthlyRevenue");
        }

        // Node RX1: Display MonthlyRevenue
        cout << "------------------------------------------\n";
        cout << "Revenue for " << setfill('0') << setw(2) << month << "/" << year << ": $"
            << fixed << setprecision(2) << monthlyRevenue << endl;
        cout << "------------------------------------------\n";

    }
    catch (SQLException& e) {
        cerr << "[Error] SQL Error fetching Monthly Revenue: " << e.what() << endl;
    }
}

// ==========================================
// 2) GENERATE TOP CUSTOMERS
// ==========================================
void generateTopCustomers(sql::Connection* con) {
    cout << "\n--- Generate Top Customers (by Jobs Created) ---\n";

    try {
        unique_ptr<Statement> stmt(con->createStatement());

        // SQL: Joins printjob and user, counts jobs per user, and limits to 5.
        // NOTE: If your 'printjob' table uses soft deletion, add "WHERE p.IsActive = TRUE"
        // to the query below to exclude deleted jobs from the count.
        unique_ptr<ResultSet> res(stmt->executeQuery(
            "SELECT u.UserID, u.FullName, COUNT(p.JobID) AS JobCount "
            "FROM printjob p "
            "JOIN user u ON p.UserID = u.UserID "
            "GROUP BY u.UserID, u.FullName "
            "ORDER BY JobCount DESC "
            "LIMIT 5"
            // Example for soft deletion: 
            // "WHERE p.IsActive = TRUE " 
        ));

        // Node TX1: Display Top 5 Customers

        // Define column widths for better visual alignment
        const int RANK_W = 7;
        const int USER_W = 12;
        const int NAME_W = 35; // Generous width for names
        const int JOBS_W = 10;
        const int TOTAL_W = RANK_W + USER_W + NAME_W + JOBS_W + 5; // Sum of widths + 4 dividers

        cout << "+" << string(TOTAL_W - 2, '-') << "+" << endl;
        cout << "| " << left << setw(TOTAL_W - 4) << "Top 5 Customers by Number of Print Jobs Created" << " |\n";
        cout << "+" << string(TOTAL_W - 2, '-') << "+" << endl;

        // Header Row
        cout << "| " << left << setw(RANK_W - 2) << "Rank" << " | "
            << left << setw(USER_W - 2) << "User ID" << " | "
            << left << setw(NAME_W - 2) << "Customer Name" << " | "
            << left << setw(JOBS_W - 2) << "Jobs" << " |\n";
        cout << "+" << string(TOTAL_W - 2, '-') << "+" << endl;

        int rank = 1;
        bool found = false;
        while (res->next()) {
            found = true;
            // Data Rows
            cout << "| " << left << setw(RANK_W - 2) << rank++ << " | "
                << left << setw(USER_W - 2) << res->getInt("UserID") << " | "
                << left << setw(NAME_W - 2) << res->getString("FullName") << " | "
                << left << setw(JOBS_W - 2) << res->getInt("JobCount") << " |\n";
        }
        cout << "+" << string(TOTAL_W - 2, '-') << "+" << endl;

        if (!found) {
            cout << "No print jobs found to rank customers.\n";
        }

    }
    catch (SQLException& e) {
        cerr << "[Error] SQL Error fetching Top Customers: " << e.what() << endl;
    }
}

// ==========================================
// MAIN MENU LOOP
// ==========================================

void runReportGenerationModule(sql::Connection* con) {
    int choice;
    do {
        // Node A: Display Report Generation options
        cout << "\n=====================================\n";
        cout << "   Report Generation Module\n";
        cout << "=====================================\n";
        cout << "1. Generate Monthly Revenue\n";
        cout << "2. Generate Top Customers (by Job Count)\n";
        cout << "3. Exit\n";
        cout << "=====================================\n";

        // Node B: Get choice
        choice = readInt("Enter your choice (1-3): ");

        // Node C, D1, D2, D3 logic
        switch (choice) {
        case 1: generateMonthlyRevenue(con); break;
        case 2: generateTopCustomers(con); break;
        case 3: cout << "Exiting Report Generation Module...\n"; break; // Node EXIT
        default: cout << "[Error] Invalid option\n"; break; // Node X0, X2
        }

    } while (choice != 3); // Node END
}