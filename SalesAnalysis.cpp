#include "SalesAnalysis.h"
#include "utils.h" // Assumes readInt, clearScreen, etc.
#include <iostream>
#include <iomanip>
#include <limits>
#include <memory>
#include <stdexcept>

using namespace std;
using namespace sql;

// ==========================================
// HELPER FUNCTION (Shared by Option 1 & 2)
// ==========================================

// Node OC1 / P2: Fetch/Compute OperationCost = SUM(QuantityUsed × UnitCost)
double fetchOperationCost(sql::Connection* con) {
    // SQL Query: Joins consumption_log and inventory to multiply QuantityUsed by UnitCost and sum the results.
    // NOTE: This assumes consumption_log and inventory are the only cost sources.
    const char* sql =
        "SELECT SUM(cl.QuantityUsed * i.UnitCost) AS OperationCost "
        "FROM inventoryconsumption cl "
        "JOIN inventory i ON cl.InventoryID = i.InventoryID";

    try {
        unique_ptr<Statement> stmt(con->createStatement());
        unique_ptr<ResultSet> res(stmt->executeQuery(sql));

        if (res->next()) {
            return res->getDouble("OperationCost");
        }
    }
    catch (SQLException& e) {
        cerr << "[Error] SQL Error fetching Operation Cost: " << e.what() << endl;
    }
    return 0.0;
}

// ==========================================
// 1) CALCULATE OPERATION COST
// ==========================================
void calculateOperationCost(sql::Connection* con) {
    cout << "\n--- Calculate Operation Cost ---\n";

    // Node OC1: Fetch/Compute OperationCost
    double operationCost = fetchOperationCost(con);

    // Node OCX: Display Operation Cost
    cout << "Total Operational Cost (from consumed inventory): $"
        << fixed << setprecision(2) << operationCost << endl;
}

// ==========================================
// 2) CALCULATE PROFIT
// ==========================================
void calculateProfit(sql::Connection* con) {
    cout << "\n--- Calculate Profit ---\n";
    double totalJobCost = 0.0;
    double operationCost = 0.0;

    // Node P1: Fetch TotalJobCost = SUM(JobCost)
    try {
        unique_ptr<Statement> stmt(con->createStatement());
        unique_ptr<ResultSet> res(stmt->executeQuery("SELECT SUM(JobCost) AS TotalJobCost FROM printjob"));

        if (res->next()) {
            totalJobCost = res->getDouble("TotalJobCost");
        }
    }
    catch (SQLException& e) {
        cerr << "[Error] SQL Error fetching Total Job Cost: " << e.what() << endl;
        return;
    }

    // Node P2: Fetch OperationCost (calls helper)
    operationCost = fetchOperationCost(con);

    // Node P3: Compute Profit = TotalJobCost - OperationCost
    double profit = totalJobCost - operationCost;

    // Node PX: Display Results
    cout << left << setw(30) << "Total Revenue (Job Costs):" << "$" << fixed << setprecision(2) << totalJobCost << endl;
    cout << left << setw(30) << "(-) Total Operational Cost:" << "$" << fixed << setprecision(2) << operationCost << endl;
    cout << "------------------------------------------\n";
    cout << left << setw(30) << "Net Profit:" << "$" << fixed << setprecision(2) << profit << endl;
}


// ==========================================
// 3) CALCULATE REVENUE (TOTAL)
// ==========================================
void calculateTotalRevenue(sql::Connection* con) {
    cout << "\n--- Calculate Total Revenue ---\n";
    double revenue = 0.0;

    // Node R1: Fetch Revenue = SUM(Amount WHERE Status = 'Complete')
    // NOTE: This assumes PaymentStatus is the column name based on your previous fix.
    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("SELECT SUM(Amount) AS TotalRevenue FROM payment WHERE PaymentStatus = 'Complete'")
        );
        unique_ptr<ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            revenue = res->getDouble("TotalRevenue");
        }
    }
    catch (SQLException& e) {
        cerr << "[Error] SQL Error fetching Revenue: " << e.what() << endl;
        return;
    }

    // Node RX: Display Revenue
    cout << "Total Revenue (Sum of ALL 'Complete' Payments): $"
        << fixed << setprecision(2) << revenue << endl;
}

// ==========================================
// MAIN MENU LOOP
// ==========================================

void runSalesAnalysisModule(sql::Connection* con) {
    int choice;
    do {
        // Node A: Display Sales Analysis options
        cout << "\n=====================================\n";
        cout << "   Sales Analysis Module\n";
        cout << "=====================================\n";
        cout << "1. Calculate Operation Cost\n";
        cout << "2. Calculate Profit\n";
        cout << "3. Calculate Total Revenue\n";
        cout << "4. Exit\n";
        cout << "=====================================\n";

        // Node B: Get choice
        choice = readInt("Enter your choice (1-4): ");

        // Node C, D1, D2, D3, D4 logic
        switch (choice) {
        case 1: calculateOperationCost(con); break;
        case 2: calculateProfit(con); break;
        case 3: calculateTotalRevenue(con); break;
        case 4: cout << "Exiting Sales Analysis Module...\n"; break; // Node EXIT
        default: cout << "[Error] Invalid option\n"; break; // Node X0, X2
        }

    } while (choice != 4); // Node END
}