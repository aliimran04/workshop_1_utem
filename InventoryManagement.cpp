#include "InventoryManagement.h"
#include "utils.h" // Assumes readInt, clearScreen, etc.
#include <iostream>
#include <iomanip>
#include <limits>
#include <memory>

using namespace std;
using namespace sql;

// ==========================================
// HELPER FUNCTIONS
// ==========================================

bool checkInventoryIDExists(sql::Connection* con, int inventoryID) {
    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("SELECT 1 FROM inventory WHERE InventoryID = ? LIMIT 1")
        );
        pstmt->setInt(1, inventoryID);
        unique_ptr<ResultSet> res(pstmt->executeQuery());
        return res->next();
    }
    catch (SQLException& e) {
        cerr << "DB Error (Inventory Check): " << e.what() << endl;
        return false;
    }
}

// ==========================================
// DISPLAY FUNCTION
// ==========================================

/*void readAllInventory(sql::Connection* con) {
    try {
        unique_ptr<Statement> stmt(con->createStatement());
        unique_ptr<ResultSet> res(stmt->executeQuery(
            "SELECT InventoryID, ItemType, Quantity, UnitCost, TimeStamp FROM inventory ORDER BY InventoryID ASC"
        ));

        const int ID_W = 6;
        const int TYPE_W = 30;
        const int QTY_W = 10;
        const int COST_W = 10;
        const int DATE_W = 20;
        const int TOTAL_WIDTH = ID_W + TYPE_W + QTY_W + COST_W + DATE_W + 6;

        cout << "\n--- All Existing Inventory Items ---\n";
        cout << "+" << string(TOTAL_WIDTH - 2, '-') << "+" << endl;
        cout << "| "
            << left << setw(ID_W - 2) << "ID" << " | "
            << left << setw(TYPE_W - 2) << "Item Type" << " | "
            << left << setw(QTY_W - 2) << "Quantity" << " | "
            << left << setw(COST_W - 2) << "Cost/Unit" << " | "
            << left << setw(DATE_W - 2) << "Last Update" << " |"
            << endl;
        cout << "+" << string(TOTAL_WIDTH - 2, '-') << "+" << endl;

        bool found = false;
        while (res->next()) {
            found = true;
            cout << "| "
                << left << setw(ID_W - 2) << res->getInt("InventoryID") << " | "
                << left << setw(TYPE_W - 2) << res->getString("ItemType") << " | "
                << left << setw(QTY_W - 2) << res->getInt("Quantity") << " | "
                << left << setw(COST_W - 2) << fixed << setprecision(2) << res->getDouble("UnitCost") << " | "
                << left << setw(DATE_W - 2) << res->getString("TimeStamp") << " |"
                << endl;
        }
        cout << "+" << string(TOTAL_WIDTH - 2, '-') << "+" << endl;

        if (!found) {
            cout << "No items found in inventory.\n";
        }
    }
    catch (SQLException& e) {
        cerr << "Error retrieving inventory: " << e.what() << endl;
    }
}*/
//test new read function
void readAllInventory(sql::Connection* con) {
    try {
        unique_ptr<Statement> stmt(con->createStatement());
        // SQL Logic: Initial = Current Quantity + Sum of Consumption Logs
        // Left = Current Quantity in 'inventory' table
        unique_ptr<ResultSet> res(stmt->executeQuery(
            "SELECT i.InventoryID, i.ItemType, i.Quantity AS QuantityLeft, "
            "IFNULL(SUM(ic.QuantityUsed), 0) AS TotalConsumed, "
            "(i.Quantity + IFNULL(SUM(ic.QuantityUsed), 0)) AS InitialEstimate, "
            "i.UnitCost, i.TimeStamp "
            "FROM inventory i "
            "LEFT JOIN inventoryconsumption ic ON i.InventoryID = ic.InventoryID "
            "GROUP BY i.InventoryID ORDER BY i.InventoryID ASC"
        ));

        const int ID_W = 6, TYPE_W = 20, QTY_W = 12, CONS_W = 12, INIT_W = 12;
        const int TOTAL_WIDTH = 85;

        cout << "\n--- Inventory Status Report ---\n";
        cout << string(TOTAL_WIDTH, '-') << endl;
        cout << "| " << left << setw(ID_W - 2) << "ID"
            << " | " << setw(TYPE_W - 2) << "Item Type"
            << " | " << setw(INIT_W - 2) << "Initial"
            << " | " << setw(CONS_W - 2) << "Consumed"
            << " | " << setw(QTY_W - 2) << "Left" << " |" << endl;
        cout << string(TOTAL_WIDTH, '-') << endl;

        while (res->next()) {
            cout << "| " << left << setw(ID_W - 2) << res->getInt("InventoryID")
                << " | " << setw(TYPE_W - 2) << res->getString("ItemType")
                << " | " << setw(INIT_W - 2) << res->getInt("InitialEstimate")
                << " | " << setw(CONS_W - 2) << res->getInt("TotalConsumed")
                << " | " << setw(QTY_W - 2) << res->getInt("QuantityLeft") << " |" << endl;
        }
        cout << string(TOTAL_WIDTH, '-') << endl;
    }
    catch (SQLException& e) {
        cerr << "Error retrieving inventory: " << e.what() << endl;
    }
}

// ==========================================
// 1) ADD INVENTORY
// ==========================================
void addInventory(sql::Connection* con) {
    cout << "\n--- Add New Inventory Item ---\n";
    string itemType;
    int quantity;
    double unitCost;

    // Node ADD1: Get inputs
    cout << "Enter Item Type (e.g., Paper, Ink): ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Ensure buffer is clear before getline
    getline(cin, itemType);
    quantity = readInt("Enter Initial Quantity: ");

    cout << "Enter Unit Cost: ";
    while (!(cin >> unitCost)) {
        cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid cost. Try again: ";
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    // Node ADD2: Quantity >= 0 and UnitCost >= 0?
    if (quantity < 0 || unitCost < 0) {
        cout << "[Error] Invalid Operation: Quantity and Unit Cost must be non-negative.\n"; // Node AX1
        return;
    }

    // Node ADD3: Insert
    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement(
                "INSERT INTO inventory (ItemType, Quantity, UnitCost) VALUES (?, ?, ?)"
            )
        );
        pstmt->setString(1, itemType);
        pstmt->setInt(2, quantity);
        pstmt->setDouble(3, unitCost);
        pstmt->executeUpdate();

        cout << "[Success] New Inventory Recorded.\n"; // Node AX2
    }
    catch (SQLException& e) {
        cerr << "[Error] SQL Error: " << e.what() << endl;
    }
}

// ==========================================
// 2) RECORD INVENTORY TOP UP
// ==========================================
void recordInventoryTopUp(sql::Connection* con) {
    cout << "\n--- Record Inventory Top Up ---\n";
    readAllInventory(con);
    int inventoryID = readInt("Enter Inventory ID to top up: ");
    int topUpQuantity;

    // Node T2: InventoryID exist?
    if (!checkInventoryIDExists(con, inventoryID)) {
        cout << "[Error] InventoryID Not Exist.\n"; // Node TX1
        return;
    }

    // Node T1: Get topUpQuantity
    topUpQuantity = readInt("Enter Top Up Quantity: ");

    // Node T3: topUpQuantity > 0?
    if (topUpQuantity <= 0) {
        cout << "[Error] Invalid Operation: Top up quantity must be positive.\n"; // Node TX2
        return;
    }

    // Node T4: Update inventory
    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement(
                "UPDATE inventory SET Quantity = Quantity + ? WHERE InventoryID = ?"
            )
        );
        pstmt->setInt(1, topUpQuantity);
        pstmt->setInt(2, inventoryID);
        pstmt->executeUpdate();

        cout << "[Success] Item Restocked Successfully.\n"; // Node TX3
    }
    catch (SQLException& e) {
        cerr << "[Error] SQL Error: " << e.what() << endl;
    }
}

// ==========================================
// 3) RECORD INVENTORY CONSUMPTION
// ==========================================
void recordInventoryConsumption(sql::Connection* con) {
    cout << "\n--- Record Inventory Consumption ---\n";
    readAllInventory(con);
    int inventoryID = readInt("Enter Inventory ID consumed: ");
    int quantityUsed;

    // Node C2: InventoryID exist?
    if (!checkInventoryIDExists(con, inventoryID)) {
        cout << "[Error] InventoryID Not Exist.\n"; // Node CX1
        return;
    }

    // Node C1: Get QuantityUsed
    quantityUsed = readInt("Enter Quantity Used: ");

    // Node C3: QuantityUsed > 0?
    if (quantityUsed <= 0) {
        cout << "[Error] Invalid Operation: Consumption quantity must be positive.\n"; // Node CX2
        return;
    }

    // Fetch current Quantity
    int currentQuantity = 0;
    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("SELECT Quantity FROM inventory WHERE InventoryID = ?")
        );
        pstmt->setInt(1, inventoryID);
        unique_ptr<ResultSet> res(pstmt->executeQuery());
        if (res->next()) {
            currentQuantity = res->getInt("Quantity");
        }
    }
    catch (SQLException& e) {
        cerr << "DB Error fetching quantity: " << e.what() << endl;
        return;
    }

    // Node C4: Quantity >= QuantityUsed?
    if (currentQuantity < quantityUsed) {
        cout << "[Error] Insufficient Stock. Current stock: " << currentQuantity << "\n"; // Node CX3
        return;
    }

    // Node C5: Update inventory
    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement(
                "UPDATE inventory SET Quantity = Quantity - ? WHERE InventoryID = ?"
            )
        );
        pstmt->setInt(1, quantityUsed);
        pstmt->setInt(2, inventoryID);
        pstmt->executeUpdate();

        // Node C6: Record consumption (optional logging table)
        unique_ptr<PreparedStatement> logPstmt(
            con->prepareStatement(
                "INSERT INTO inventoryconsumption (InventoryID, QuantityUsed) VALUES (?, ?)"
            )
        );
        logPstmt->setInt(1, inventoryID);
        logPstmt->setInt(2, quantityUsed);
        logPstmt->executeUpdate();

        cout << "[Success] Consumption Recorded.\n"; // Node CX4
    }
    catch (SQLException& e) {
        cerr << "[Error] SQL Error during update/logging: " << e.what() << endl;
    }
}

// ==========================================
// 4) SEARCH INVENTORY INFO
// ==========================================
/*void searchInventory(sql::Connection* con) {
    cout << "\n--- Search Inventory Info ---\n";
    readAllInventory(con);
    int inventoryID = readInt("Enter Inventory ID to search: ");

    // Node S2: InventoryID exist?
    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT ItemType, Quantity, UnitCost, TimeStamp FROM inventory WHERE InventoryID = ?"
            )
        );
        pstmt->setInt(1, inventoryID);
        unique_ptr<ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            // Node SX2: Display Inventory Info
            cout << "\n--- Details for Inventory ID " << inventoryID << " ---\n";
            cout << left << setw(15) << "Item Type:" << res->getString("ItemType") << endl;
            cout << left << setw(15) << "Quantity:" << res->getInt("Quantity") << endl;
            cout << left << setw(15) << "Unit Cost:" << fixed << setprecision(2) << res->getDouble("UnitCost") << endl;
            cout << left << setw(15) << "Last Update:" << res->getString("TimeStamp") << endl;
            cout << "--------------------------------\n";
        }
        else {
            cout << "[Error] InventoryID Not Exist.\n"; // Node SX1
        }
    }
    catch (SQLException& e) {
        cerr << "[Error] SQL Error: " << e.what() << endl;
    }
}*/

//test new search function including consumed inventory and how many left
void searchInventory(sql::Connection* con) {
    cout << "\n--- Detailed Inventory Search ---\n";
    readAllInventory(con);
    int inventoryID = readInt("Enter Inventory ID: ");

    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT i.ItemType, i.Quantity AS QuantityLeft, i.UnitCost, i.TimeStamp, "
                "IFNULL(SUM(ic.QuantityUsed), 0) AS TotalConsumed "
                "FROM inventory i "
                "LEFT JOIN inventoryconsumption ic ON i.InventoryID = ic.InventoryID "
                "WHERE i.InventoryID = ? GROUP BY i.InventoryID"
            )
        );
        pstmt->setInt(1, inventoryID);
        unique_ptr<ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            int leftVal = res->getInt("QuantityLeft");
            int consumed = res->getInt("TotalConsumed");
            int initial = leftVal + consumed;

            cout << "\n--- Details for " << res->getString("ItemType") << " ---\n";

            // FIX: Ensure setw(20) is placed immediately before the text string
            cout << setw(20) << left << "Total Consumed:" << consumed << endl;
            cout << setw(20) << left << "Quantity Left:" << leftVal << endl;
            cout << setw(20) << left << "Initial Stock:" << initial << " (Approx.)" << endl;
            cout << setw(20) << left << "Unit Cost:" << "$" << fixed << setprecision(2) << res->getDouble("UnitCost") << endl;
            cout << setw(20) << left << "Last Updated:" << res->getString("TimeStamp") << endl;
        }
        else {
            cout << "[Error] InventoryID " << inventoryID << " not found.\n";
        }
    }
    catch (SQLException& e) {
        cerr << "[Error] SQL Error: " << e.what() << endl;
    }
}

// ==========================================
// MAIN MENU LOOP
// ==========================================

void runInventoryModule(sql::Connection* con) {
    int choice;
    do {
        // Node A: Display Inventory Management options
        cout << "\n=====================================\n";
        cout << "   Inventory Management Module\n";
        cout << "=====================================\n";
        cout << "1. Add New Inventory Item\n";
        cout << "2. Record Inventory Top Up\n";
        cout << "3. Record Inventory Consumption\n";
        cout << "4. Search Inventory Info\n";
        cout << "5. Exit\n";
        cout << "=====================================\n";

        // Node B: Get choice
        choice = readInt("Enter your choice (1-5): ");

        // Node C, D1, D2, D3, D4, D5 logic
        switch (choice) {
        case 1: addInventory(con); break;
        case 2: recordInventoryTopUp(con); break;
        case 3: recordInventoryConsumption(con); break;
        case 4: searchInventory(con); break;
        case 5: cout << "Exiting Inventory Module...\n"; break; // Node EXIT
        default: cout << "[Error] Invalid option\n"; break; // Node X0
        }

    } while (choice != 5); // Node END
}