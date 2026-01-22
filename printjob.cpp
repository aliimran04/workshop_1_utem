#include "printjob.h"
#include "utils.h" // For readInt, cin.ignore, clearScreen (assuming it's here)
#include <iostream>
#include <limits>
#include <iomanip>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h> // Include for statement and last_insert_id

using namespace std;
using namespace sql;

// --- Helper Functions (No Change, but assumed isCustomerUser is defined elsewhere) ---
bool isCustomerUser(Connection* con, int userID) {
    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("SELECT 1 FROM user WHERE UserID = ? AND Role = 'Customer' LIMIT 1")
        );
        pstmt->setInt(1, userID);
        unique_ptr<ResultSet> res(pstmt->executeQuery());
        return res->next(); // True only if user is a Customer
    }
    catch (SQLException& e) {
        cerr << "Database Error (Customer User Check): " << e.what() << endl;
        return false;
    }
}

bool doesUserExist(sql::Connection* con, int userID) {
    // NOTE: This should ideally be replaced by isCustomerUser for job creation context
    try {
        unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("SELECT 1 FROM user WHERE UserID = ? LIMIT 1")
        );
        pstmt->setInt(1, userID);
        unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        return res->next(); // True if user exists
    }
    catch (sql::SQLException& e) {
        cerr << "Database Error (User Check): " << e.what() << endl;
        return false;
    }
}

bool doesJobExist(sql::Connection* con, int jobID) {
    try {
        unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("SELECT 1 FROM printjob WHERE JobID = ? LIMIT 1")
        );
        pstmt->setInt(1, jobID);
        unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        return res->next(); // True if job exists
    }
    catch (sql::SQLException& e) {
        cerr << "Database Error (Job Check): " << e.what() << endl;
        return false;
    }
}

bool isInventorySufficient(sql::Connection* con, int pageCount) {
    try {
        // Query both Paper and Ink quantities
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("SELECT ItemType, Quantity FROM inventory WHERE ItemType IN ('Paper', 'Ink')")
        );

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        int paperLeft = 0;
        int inkLeft = 0;

        while (res->next()) {
            std::string type = res->getString("ItemType");
            if (type == "Paper") paperLeft = res->getInt("Quantity");
            if (type == "Ink") inkLeft = res->getInt("Quantity");
        }

        // Logic: 1 unit of Ink is required for every 100 pages (Adjust ratio as needed)
        int inkRequired = (pageCount + 99) / 100;

        if (paperLeft < pageCount) {
            std::cout << "[Error] Insufficient Paper. Need: " << pageCount << ", Have: " << paperLeft << std::endl;
            return false;
        }
        if (inkLeft < inkRequired) {
            std::cout << "[Error] Insufficient Ink. Need: " << inkRequired << " units, Have: " << inkLeft << std::endl;
            return false;
        }

        return true;
    }
    catch (sql::SQLException& e) {
        std::cerr << "Database Error: " << e.what() << std::endl;
        return false;
    }
}

// --- Display Functions (Assume logic includes JOIN for readPrintJobs elsewhere) ---

// readCustomers and readPrintJobs definitions assumed to be correct based on previous steps

// --- CRUD Operations (Fix: Remove redundant list calls) ---

/*void createPrintJob(sql::Connection* con, int userID, int pageCount, double costPerPage) {
    int newJobID = -1;
    // REMOVED: readCustomers(con); // List displayed at start of menu loop

    try {
        unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "INSERT INTO printjob (UserID, PageCount, CostPerPage,  TimeStamp) "
                "VALUES (?, ?, ?, NOW())"
            )
        );
        // ... (rest of insertion logic is fine)
        pstmt->setInt(1, userID);
        pstmt->setInt(2, pageCount);
        pstmt->setDouble(3, costPerPage);
        pstmt->executeUpdate();

        // --- 2. Retrieve the Last Inserted ID (JobID) ---
        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID() AS last_id"));
        if (res->next()) {
            newJobID = res->getInt("last_id");
        }

        // --- 3. Query the generated JobCost using the JobID ---
        if (newJobID != -1) {
            std::unique_ptr<sql::PreparedStatement> costPstmt(
                con->prepareStatement("SELECT JobCost FROM printjob WHERE JobID = ?")
            );
            costPstmt->setInt(1, newJobID);
            std::unique_ptr<sql::ResultSet> costRes(costPstmt->executeQuery());

            if (costRes->next()) {
                double jobCost = costRes->getDouble("JobCost");
                std::cout << "Print Job recorded successfully! JobID: " << newJobID
                    << ", Calculated Cost: $" << std::fixed << std::setprecision(2) << jobCost << std::endl;
            }
            else {
                std::cout << " Print Job recorded successfully (JobID: " << newJobID << "), but failed to retrieve cost." << std::endl;
            }
        }
        else {
            std::cout << " Print Job recorded successfully, but could not retrieve JobID." << std::endl;
        }

    }
    catch (sql::SQLException& e) {
        cerr << "Error creating print job: " << e.what() << endl;
    }
}*/
//test cretae print job with auto consumption 
void createPrintJob(sql::Connection* con, int userID, int pageCount, double costPerPage) {
    int newJobID = -1;

    try {
        // 1. Insert the Print Job into the database
        unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "INSERT INTO printjob (UserID, PageCount, CostPerPage, TimeStamp) "
                "VALUES (?, ?, ?, NOW())"
            )
        );
        pstmt->setInt(1, userID);
        pstmt->setInt(2, pageCount);
        pstmt->setDouble(3, costPerPage);
        pstmt->executeUpdate();

        // Retrieve the generated JobID
        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID() AS last_id"));
        if (res->next()) {
            newJobID = res->getInt("last_id");
        }

        if (newJobID != -1) {
            // 2. Record Consumption for Paper
            // Update inventory Quantity (Stock Left)
            unique_ptr<sql::PreparedStatement> paperUpdate(
                con->prepareStatement("UPDATE inventory SET Quantity = Quantity - ? WHERE ItemType = 'Paper'")
            );
            paperUpdate->setInt(1, pageCount);
            paperUpdate->executeUpdate();

            // Log usage for reports (Total Consumed)
            unique_ptr<sql::PreparedStatement> paperLog(
                con->prepareStatement("INSERT INTO inventoryconsumption (InventoryID, QuantityUsed) SELECT InventoryID, ? FROM inventory WHERE ItemType = 'Paper'")
            );
            paperLog->setInt(1, pageCount);
            paperLog->executeUpdate();

            // 3. Record Consumption for Ink
            // Use the same ratio (1 unit per 100 pages) as isInventorySufficient
            int inkUsed = (pageCount + 99) / 100;

            unique_ptr<sql::PreparedStatement> inkUpdate(
                con->prepareStatement("UPDATE inventory SET Quantity = Quantity - ? WHERE ItemType = 'Ink'")
            );
            inkUpdate->setInt(1, inkUsed);
            inkUpdate->executeUpdate();

            unique_ptr<sql::PreparedStatement> inkLog(
                con->prepareStatement("INSERT INTO inventoryconsumption (InventoryID, QuantityUsed) SELECT InventoryID, ? FROM inventory WHERE ItemType = 'Ink'")
            );
            inkLog->setInt(1, inkUsed);
            inkLog->executeUpdate();

            // 4. Retrieve and display JobCost
            std::unique_ptr<sql::PreparedStatement> costPstmt(
                con->prepareStatement("SELECT JobCost FROM printjob WHERE JobID = ?")
            );
            costPstmt->setInt(1, newJobID);
            std::unique_ptr<sql::ResultSet> costRes(costPstmt->executeQuery());

            if (costRes->next()) {
                double jobCost = costRes->getDouble("JobCost");
                std::cout << "\n[Success] Print Job & Consumption recorded!" << std::endl;
                std::cout << "JobID: " << newJobID << " | Calculated Cost: $" << std::fixed << std::setprecision(2) << jobCost << std::endl;
                std::cout << "Materials Used: " << pageCount << " pages and " << inkUsed << " units of ink." << std::endl;
            }
        }
    }
    catch (sql::SQLException& e) {
        cerr << "Error in createPrintJob/Consumption: " << e.what() << endl;
    }
}

/*void updatePrintJob(sql::Connection* con, int jobID, int newPageCount, double newCostPerPage) {
    // REMOVED: readPrintJobs(con); // List displayed at start of menu loop
    try {
        // ... (rest of update logic is fine)
        std::string sql = "UPDATE printjob SET ";
        int setCount = 0;
        // ... build sql, set parameters, execute ...

        if (newPageCount > 0) {
            sql += "PageCount=?, ";
            setCount++;
        }

        if (newCostPerPage > 0.0) {
            sql += "CostPerPage=?, ";
            setCount++;
        }

        if (setCount > 0) {
            sql.erase(sql.length() - 2);
            sql += " WHERE JobID=?";

            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(sql));
            int idx = 1;

            if (newPageCount > 0) pstmt->setInt(idx++, newPageCount);
            if (newCostPerPage > 0.0) pstmt->setDouble(idx++, newCostPerPage);

            pstmt->setInt(idx++, jobID);

            int affected = pstmt->executeUpdate();

            if (affected > 0) {
                std::cout << "Print Job updated successfully!" << std::endl;
            }
            else {
                std::cout << "No changes made, or JobID not found." << std::endl;
            }
        }
        else {
            std::cout << "No new data. No changes made." << std::endl;
        }

    }
    catch (sql::SQLException& e) {
        std::cerr << "Error updating print job: " << e.what() << std::endl;
    }
}*/
//test update print job with auto consumption
/*void updatePrintJob(sql::Connection* con, int jobID, int newPageCount, double newCostPerPage) {
    try {
        // 1. Fetch the OLD PageCount to calculate the inventory difference
        int oldPageCount = 0;
        unique_ptr<PreparedStatement> selectPstmt(
            con->prepareStatement("SELECT PageCount FROM printjob WHERE JobID = ?")
        );
        selectPstmt->setInt(1, jobID);
        unique_ptr<ResultSet> res(selectPstmt->executeQuery());

        if (!res->next()) {
            cout << "[Error] Job ID not found." << endl;
            return;
        }
        oldPageCount = res->getInt("PageCount");

        // 2. Determine if inventory adjustment is needed
        // Only adjust if the user provided a newPageCount ( > 0) and it's different from the old one
        int pageDiff = 0;
        int inkDiff = 0;
        if (newPageCount > 0 && newPageCount != oldPageCount) {
            pageDiff = newPageCount - oldPageCount;
            // Calculate ink difference based on your 1 unit per 100 pages ratio
            int oldInk = (oldPageCount + 99) / 100;
            int newInk = (newPageCount + 99) / 100;
            inkDiff = newInk - oldInk;

            // 3. Check if we have enough stock if the count is INCREASING
            if (pageDiff > 0) {
                if (!isInventorySufficient(con, pageDiff)) {
                    cout << "[Error] Update failed: Insufficient inventory for the increase." << endl;
                    return;
                }
            }
        }

        // 4. Build and execute the SQL Update for the printjob table
        std::string sql = "UPDATE printjob SET ";
        bool first = true;
        if (newPageCount > 0) {
            sql += "PageCount = ?";
            first = false;
        }
        if (newCostPerPage > 0.0) {
            if (!first) sql += ", ";
            sql += "CostPerPage = ?";
        }
        sql += " WHERE JobID = ?";

        unique_ptr<PreparedStatement> pstmt(con->prepareStatement(sql));
        int idx = 1;
        if (newPageCount > 0) pstmt->setInt(idx++, newPageCount);
        if (newCostPerPage > 0.0) pstmt->setDouble(idx++, newCostPerPage);
        pstmt->setInt(idx++, jobID);
        pstmt->executeUpdate();

        // 5. Adjust Inventory and Consumption Logs
        if (pageDiff != 0) {
            // Adjust Paper
            unique_ptr<PreparedStatement> paperUpd(
                con->prepareStatement("UPDATE inventory SET Quantity = Quantity - ? WHERE ItemType = 'Paper'")
            );
            paperUpd->setInt(1, pageDiff);
            paperUpd->executeUpdate();

            unique_ptr<PreparedStatement> paperLog(
                con->prepareStatement("INSERT INTO inventoryconsumption (InventoryID, QuantityUsed) SELECT InventoryID, ? FROM inventory WHERE ItemType = 'Paper'")
            );
            paperLog->setInt(1, pageDiff);
            paperLog->executeUpdate();

            // Adjust Ink (if the unit count changed)
            if (inkDiff != 0) {
                unique_ptr<PreparedStatement> inkUpd(
                    con->prepareStatement("UPDATE inventory SET Quantity = Quantity - ? WHERE ItemType = 'Ink'")
                );
                inkUpd->setInt(1, inkDiff);
                inkUpd->executeUpdate();

                unique_ptr<PreparedStatement> inkLog(
                    con->prepareStatement("INSERT INTO inventoryconsumption (InventoryID, QuantityUsed) SELECT InventoryID, ? FROM inventory WHERE ItemType = 'Ink'")
                );
                inkLog->setInt(1, inkDiff);
                inkLog->executeUpdate();
            }
            cout << "[Success] Inventory adjusted by " << pageDiff << " pages." << endl;
        }

        cout << "[Success] Print Job updated successfully!" << endl;

    }
    catch (sql::SQLException& e) {
        cerr << "Error updating print job and inventory: " << e.what() << endl;
    }
}*/
void updatePrintJob(sql::Connection* con, int jobID, int newPageCount, double newCostPerPage) {
    try {
        // 1. Fetch the OLD PageCount
        int oldPageCount = 0;
        unique_ptr<PreparedStatement> selectPstmt(
            con->prepareStatement("SELECT PageCount FROM printjob WHERE JobID = ?")
        );
        selectPstmt->setInt(1, jobID);
        unique_ptr<ResultSet> res(selectPstmt->executeQuery());

        if (!res->next()) {
            cout << "[Error] Job ID not found." << endl;
            return;
        }
        oldPageCount = res->getInt("PageCount");

        // 2. Inventory Logic (Only if pages changed)
        int pageDiff = 0;
        int inkDiff = 0;
        if (newPageCount > 0 && newPageCount != oldPageCount) {
            pageDiff = newPageCount - oldPageCount;
            int oldInk = (oldPageCount + 99) / 100;
            int newInk = (newPageCount + 99) / 100;
            inkDiff = newInk - oldInk;

            if (pageDiff > 0) {
                if (!isInventorySufficient(con, pageDiff)) {
                    cout << "[Error] Update failed: Insufficient inventory." << endl;
                    return;
                }
            }
        }

        // 3. Build SQL (THE FIX IS HERE)
        std::string sql = "UPDATE printjob SET ";
        bool anyChange = false; // changed variable name from 'first' to 'anyChange' for clarity

        if (newPageCount > 0) {
            sql += "PageCount = ?";
            anyChange = true;
        }
        if (newCostPerPage > 0.0) {
            if (anyChange) sql += ", "; // Add comma if we already added PageCount
            sql += "CostPerPage = ?";
            anyChange = true;
        }

        // --- CRITICAL CHECK ---
        // If the user skipped both inputs, stop here. Do not run SQL.
        if (!anyChange) {
            cout << "No changes requested. Update skipped." << endl;
            return;
        }

        sql += " WHERE JobID = ?";

        // 4. Execute Update
        unique_ptr<PreparedStatement> pstmt(con->prepareStatement(sql));
        int idx = 1;
        if (newPageCount > 0) pstmt->setInt(idx++, newPageCount);
        if (newCostPerPage > 0.0) pstmt->setDouble(idx++, newCostPerPage);
        pstmt->setInt(idx++, jobID);
        pstmt->executeUpdate();

        // 5. Adjust Inventory (if needed)
        if (pageDiff != 0) {
            // Adjust Paper
            unique_ptr<PreparedStatement> paperUpd(
                con->prepareStatement("UPDATE inventory SET Quantity = Quantity - ? WHERE ItemType = 'Paper'")
            );
            paperUpd->setInt(1, pageDiff);
            paperUpd->executeUpdate();

            // Adjust Ink
            if (inkDiff != 0) {
                unique_ptr<PreparedStatement> inkUpd(
                    con->prepareStatement("UPDATE inventory SET Quantity = Quantity - ? WHERE ItemType = 'Ink'")
                );
                inkUpd->setInt(1, inkDiff);
                inkUpd->executeUpdate();
            }
            cout << "[Success] Inventory adjusted." << endl;
        }

        cout << "[Success] Print Job updated successfully!" << endl;

    }
    catch (sql::SQLException& e) {
        cerr << "Error updating print job: " << e.what() << endl;
    }
}


void deletePrintJob(sql::Connection* con, int jobID) {
    // REMOVED: readPrintJobs(con); // List displayed at start of menu loop
    try {
        unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("DELETE FROM printjob WHERE JobID = ?")
        );
        pstmt->setInt(1, jobID);

        if (pstmt->executeUpdate() > 0) {
            cout << "Job Deleted successfully!" << endl;
        }
        else {
            cout << "Job Not Exist." << endl;
        }
    }
    catch (sql::SQLException& e) {
        cerr << "Error deleting print job: " << e.what() << endl;
    }
}

void searchPrintJob(sql::Connection* con, int jobID) {
    // REMOVED: readPrintJobs(con); // List displayed at start of menu loop
    try {
        // ... (rest of search logic is fine)
        unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("SELECT UserID, PageCount, TimeStamp, CostPerPage, JobCost FROM printjob WHERE JobID = ?")
        );
        pstmt->setInt(1, jobID);
        unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            cout << "--- Job Details for JobID " << jobID << " ---\n";
            cout << left << setw(15) << "User ID:" << res->getInt("UserID") << endl;
            cout << left << setw(15) << "Page Count:" << res->getInt("PageCount") << endl;
            cout << left << setw(15) << "Timestamp:" << res->getString("TimeStamp") << endl;
            cout << left << setw(15) << "Cost Per Page:" << fixed << setprecision(2) << res->getDouble("CostPerPage") << endl;
            cout << left << setw(15) << "Job Cost:" << fixed << setprecision(2) << res->getDouble("JobCost") << endl;
        }
        else {
            cout << "Job Not Exist." << endl;
        }
    }
    catch (sql::SQLException& e) {
        cerr << "Error searching print job: " << e.what() << endl;
    }
}

// readCustomers and readPrintJobs definitions assumed to be here.
// In your printjob.cpp or PrintJobManager.cpp file:
//read list of customer role user in the database
int readCustomers(sql::Connection* con) {
    int customerCount = 0;
    try {
        // Get Total Count first
        customerCount = countCustomerUsers(con);

        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("SELECT UserID, FullName, Email FROM user WHERE Role = 'Customer' ORDER BY UserID ASC")
        );
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        const int ID_W = 10, NAME_W = 25, EMAIL_W = 35;
        const int TOTAL_WIDTH = ID_W + NAME_W + EMAIL_W + 4;

        int displayed = 0;
        int pageSize = 15; // Small batch so the menu stays visible

        auto printHeader = [&]() {
            std::cout << "\n--- Available Customers (" << customerCount << " total) ---\n";
            std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
            std::cout << "| " << std::left << std::setw(ID_W - 2) << "User ID" << " | "
                << std::left << std::setw(NAME_W - 3) << "FullName" << " | "
                << std::left << std::setw(EMAIL_W - 3) << "Email" << " |" << std::endl;
            std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
            };

        printHeader();

        while (res->next()) {
            displayed++;
            std::cout << "| " << std::left << std::setw(ID_W - 2) << res->getInt("UserID") << " | "
                << std::left << std::setw(NAME_W - 3) << res->getString("FullName") << " | "
                << std::left << std::setw(EMAIL_W - 3) << res->getString("Email") << " |" << std::endl;

            if (displayed % pageSize == 0 && displayed < customerCount) {
                std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
                std::cout << ">>> [" << displayed << "/" << customerCount << "] Enter for more, 's' to skip to menu: ";
                std::string input;
                std::getline(std::cin >> std::ws, input);
                if (!input.empty() && std::tolower(input[0]) == 's') break;
                printHeader();
            }
        }
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;

    }
    catch (sql::SQLException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return customerCount;
}

int countCustomerUsers(sql::Connection* con) {
    try {
        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        // Use COUNT(*) for maximum efficiency and minimum data transfer
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
            "SELECT COUNT(UserID) AS customer_count FROM user WHERE Role = 'Customer'"
        ));

        if (res->next()) {
            // Return the count directly
            return res->getInt("customer_count");
        }
    }
    catch (sql::SQLException& e) {
        // Suppress output (remove cerr/cout lines) but return 0 in case of error
        // If necessary for debugging, you can log the error silently:
        // logError("Database Error during silent count: " + std::string(e.what())); 
    }
    return 0;
}

void readPrintJobs(sql::Connection* con) {
    try {
        // 1. Get total job count
        std::unique_ptr<sql::Statement> stmtCount(con->createStatement());
        std::unique_ptr<sql::ResultSet> resCount(stmtCount->executeQuery("SELECT COUNT(*) FROM printjob"));
        long long totalJobs = 0;
        if (resCount->next()) totalJobs = resCount->getInt64(1);

        // 2. Query data
        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
            "SELECT p.JobID, p.UserID, u.FullName, p.PageCount, p.JobCost "
            "FROM printjob p JOIN user u ON p.UserID = u.UserID "
            "ORDER BY p.JobID DESC"
        ));

        const int JOB_ID_W = 10, USER_ID_W = 10, NAME_W = 25, PAGE_W = 12, COST_W = 12;
        const int TOTAL_WIDTH = JOB_ID_W + USER_ID_W + NAME_W + PAGE_W + COST_W + 6;

        int displayed = 0;
        int pageSize = 20;

        auto printHeader = [&]() {
            std::cout << "\n--- Job History (" << totalJobs << " records) ---\n";
            std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
            std::cout << "| " << std::left << std::setw(JOB_ID_W - 2) << "Job ID" << " | "
                << std::left << std::setw(USER_ID_W - 2) << "User ID" << " | "
                << std::left << std::setw(NAME_W - 2) << "Customer" << " | "
                << std::left << std::setw(PAGE_W - 2) << "Pages" << " | "
                << std::left << std::setw(COST_W - 2) << "Cost ($)" << " |" << std::endl;
            std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
            };

        printHeader();

        while (res->next()) {
            displayed++;
            std::cout << "| " << std::left << std::setw(JOB_ID_W - 2) << res->getInt("JobID") << " | "
                << std::left << std::setw(USER_ID_W - 2) << res->getInt("UserID") << " | "
                << std::left << std::setw(NAME_W - 2) << res->getString("FullName") << " | "
                << std::left << std::setw(PAGE_W - 2) << res->getInt("PageCount") << " | "
                << std::left << std::setw(COST_W - 2) << std::fixed << std::setprecision(2) << res->getDouble("JobCost") << " |" << std::endl;

            if (displayed % pageSize == 0 && displayed < totalJobs) {
                std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
                std::cout << ">>> [" << displayed << "/" << totalJobs << "] Enter for more, 's' to stop: ";
                std::string input;
                std::getline(std::cin >> std::ws, input);
                if (!input.empty() && std::tolower(input[0]) == 's') break;
                printHeader();
            }
        }
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;

    }
    catch (sql::SQLException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}


// --- Main Menu Function (PrintJobManagementMenu) ---

/*void PrintJobManagementMenu(sql::Connection* con) {
    int choice = 0;

    // FIX: Remove initial calls outside the loop
    // REMOVED: int availableCustomers = readCustomers(con);
    // REMOVED: readPrintJobs(con);

    while (true) {

        // --- FIX: Relocate clearing and list display logic inside the loop ---
        // A
        //clearScreen(); // Must be called inside the loop

        // 1. Display customers and get the count (Relocated from outside loop)
        int availableCustomers = countCustomerUsers(con);
        readCustomers(con);
        // Display ALL existing print jobs (Relocated from outside loop)
       // readPrintJobs(con);

        // 2. Display warning if creation is blocked
        if (availableCustomers == 0) {
            std::cout << "\n WARNING: No customer user accounts found in the system. Please create Customer users first. \n";
        }

        // Display Menu
        cout << "\n=== Print Job Management ===\n";
        cout << "1. Create New Print Job\n";
        cout << "2. Update Print Job Info\n";
        cout << "3. Delete Print Job Info\n";
        cout << "4. Search Print Job Info\n";
        cout << "5. Exit\n";
        cout << "============================\n";

        // B: Get choice
        choice = readInt("Enter choice: ");
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        // C: Input Validation
        if (choice < 1 || choice > 5) {
            cout << "Invalid option. Press Enter to retry...\n";
            cin.get();
            continue;
        }

        // FIX: Remove misplaced code from switch statement
        // REMOVED: clearScreen();
        // REMOVED: readPrintJobs(con);

        switch (choice) {
        case 1: { // Create New Print Job
            

            // 2. Conditional Check: If no customers, block creation.
            if (availableCustomers == 0) {
                std::cout << "\n Cannot create a print job. No customer user accounts found.\n";
                std::cout << "   Please create a customer account using the User Management Menu.\n";
                break;
            }
            int pageCount;
            do {
                pageCount = readInt("Enter Page Count (Must be > 0): ");
                if (pageCount <= 0) {
                    cout << "Error: Page Count must be greater than zero. Please try again." << endl;
                    // The loop continues if pageCount is invalid
                }
            } while (pageCount <= 0);

            // C1: Get remaining inputs
            int userID = readInt("Enter User ID from the list above: ");
            //pageCount = readInt("Enter Page Count: ");
            double costPerPage = 0.50; // Assuming a default value

            // --- ADDED VALIDATION: Page Count must be positive ---
            if (pageCount <= 0) {
                cout << "Page Count must be greater than zero. Print job creation aborted." << endl;
                break;
            }

            // C2: UID exist? (Should ideally check for Customer role via isCustomerUser)
            // Using the provided doesUserExist, but note this is less secure:
            if (!doesUserExist(con, userID)) {
                cout << "User Not Exist." << endl;
                break;
            }

            // C4: Inventory sufficient?
            if (!isInventorySufficient(con, pageCount)) {
                cout << "Print Job can't be recorded: Inventory insufficient." << endl;
                break;
            }

            createPrintJob(con, userID, pageCount, costPerPage);
            break;
        }
        case 2: { // Update Print Job Info
            readPrintJobs(con);
            int jobID = readInt("Enter Job ID to update: ");
            if (!doesJobExist(con, jobID)) { cout << "Job Not Exist." << endl; break; }

            cout << "\n--- Enter new values (0 for count/cost to skip) ---\n";
            int newPageCount = readInt("New Page Count (0 to skip): ");
            double newCostPerPage = 0.0;
            cout << "New Cost Per Page (0.0 to skip): ";
            cin >> newCostPerPage;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            updatePrintJob(con, jobID, newPageCount, newCostPerPage);
            break;
        }
        case 3: { // Delete Print Job Info
            readPrintJobs(con);
            int jobID = readInt("Enter Job ID to delete: ");
            if (!doesJobExist(con, jobID)) { cout << "Job Not Exist." << endl; break; }

            deletePrintJob(con, jobID);
            break;
        }
        case 4: { // Search Print Job Info

            // 1. Display the existing jobs list first
            readPrintJobs(con);

            // 2. Read input
            int jobID = readInt("Enter Job ID to search: ");

            // 3. Explicitly clear the buffer after reading jobID
            // This ensures the final cin.get() command won't skip.
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            // 4. Perform the search and display details
            searchPrintJob(con, jobID);

            // REMOVED: clearScreen(); 
            // The screen should only be cleared at the START of the main loop, 
            // not during a function call, or it makes the user experience confusing.

            break;
        }
        case 5: { // Exit
            cout << "Exiting Print Job Management Menu." << endl;
            return;
        }
        default:
            cout << "Internal Error. Press Enter to retry...\n";
            cin.get();
            break;
        }

        // Wait for user before looping back (A)
        cout << "\nPress Enter to return to Print Job Menu...\n";
        cin.get();
    }
} */
// Helper 1: Search for users by name and display them
bool searchUsersByName(sql::Connection* con, string nameInput) {
    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("SELECT UserID, FullName, Email FROM user WHERE FullName LIKE ? AND Role = 'Customer'")
        );
        pstmt->setString(1, "%" + nameInput + "%");
        unique_ptr<ResultSet> res(pstmt->executeQuery());

        if (!res->isBeforeFirst()) { // Check if result set is empty
            cout << "No users found matching \"" << nameInput << "\"." << endl;
            return false;
        }

        cout << "\n--- Search Results for \"" << nameInput << "\" ---\n";
        cout << left << setw(10) << "UserID" << setw(25) << "Full Name" << setw(30) << "Email" << endl;
        cout << "----------------------------------------------------------------" << endl;

        while (res->next()) {
            cout << left << setw(10) << res->getInt("UserID")
                << setw(25) << res->getString("FullName")
                << setw(30) << res->getString("Email") << endl;
        }
        cout << "----------------------------------------------------------------" << endl;
        return true;
    }
    catch (sql::SQLException& e) {
        cerr << "Error searching users: " << e.what() << endl;
        return false;
    }
}

// Helper 2: List all jobs for a specific UserID
bool listJobsForUser(sql::Connection* con, int userID) {
    try {
        // We still fetch the data, ordered by newest first
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("SELECT JobID, PageCount, TimeStamp, JobCost FROM printjob WHERE UserID = ? ORDER BY TimeStamp DESC")
        );
        pstmt->setInt(1, userID);
        unique_ptr<ResultSet> res(pstmt->executeQuery());

        if (!res->isBeforeFirst()) {
            cout << "No print jobs found for User ID: " << userID << endl;
            return false;
        }

        cout << "\n--- Job History for User ID " << userID << " ---\n";
        cout << left << setw(10) << "JobID" << setw(15) << "Pages" << setw(12) << "Cost" << setw(25) << "Date/Time" << endl;
        cout << "--------------------------------------------------------------" << endl;

        int rowCount = 0;
        int pageSize = 10; // Adjust this number to change how many show at once

        while (res->next()) {
            cout << left << setw(10) << res->getInt("JobID")
                << setw(15) << res->getInt("PageCount")
                << "$" << setw(11) << fixed << setprecision(2) << res->getDouble("JobCost")
                << setw(25) << res->getString("TimeStamp") << endl;

            rowCount++;

            // --- Pagination Logic ---
            // If we have shown 'pageSize' rows, pause the loop
            if (rowCount % pageSize == 0) {
                cout << "--------------------------------------------------------------" << endl;
                cout << ">>> Showing " << rowCount << " jobs. Found your Job ID?" << endl;
                cout << ">>> Press [Enter] for older jobs, or type 's' to stop listing: ";

                string input;
                getline(cin, input);

                if (!input.empty() && tolower(input[0]) == 's') {
                    // User found what they wanted, break the display loop
                    cout << ">>> Listing stopped." << endl;
                    break;
                }

                // If they pressed Enter, reprint the header for clarity
                cout << "\n--- Page " << (rowCount / pageSize) + 1 << " ---\n";
                cout << left << setw(10) << "JobID" << setw(15) << "Pages" << setw(12) << "Cost" << setw(25) << "Date/Time" << endl;
                cout << "--------------------------------------------------------------" << endl;
            }
        }
        cout << "--------------------------------------------------------------" << endl;
        return true;
    }
    catch (sql::SQLException& e) {
        cerr << "Error listing jobs: " << e.what() << endl;
        return false;
    }
}

void PrintJobManagementMenu(sql::Connection* con) {
    while (true) {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif

        int availableCustomers = countCustomerUsers(con);

        cout << "\n=== Print Job Management (Total Customers: " << availableCustomers << ") ===\n";
        cout << "1. Create New Print Job\n";
        cout << "2. Update Print Job Info\n";
        cout << "3. Delete Print Job Info\n";
        cout << "4. Search Print Job Info\n";
        cout << "5. Exit\n";
        cout << "================================================\n";

        int choice = readInt("Enter choice: ");
        // Ensure buffer is clean before the next possible getline
        std::cin.ignore(numeric_limits<streamsize>::max(), '\n');

       /* switch (choice) {
        case 1:
            if (availableCustomers > 0) {
                // Option: Let user search for customer ID by name first
                while (!searchIDsByCustomerName(con));
                int userID = readInt("Enter User ID to assign job: ");
                int pageCount = readInt("Enter Page Count: ");
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                if (isInventorySufficient(con, pageCount)) {
                    createPrintJob(con, userID, pageCount, 0.50);
                }
            }
            break;

        case 2: { // Update
            // 1. Loop search until a valid name is found
            while (!searchIDsByCustomerName(con));

            // 2. Capture the specific Job ID from the displayed list
            int jobID = readInt("Enter Job ID from the list above to update: ");

            // 3. Capture the new data
            int newPages = readInt("New Page Count (0 to skip): ");

            // 4. Pass the variables to the update function
            updatePrintJob(con, jobID, newPages, 0.0);

            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }
        case 3: {// Delete
            // 1. Force search until a valid customer name is found
            while (!searchIDsByCustomerName(con));

            // 2. Prompt for the specific Job ID identified in the search results
            int jobIDToDelete = readInt("Enter Job ID to Delete: ");

            // 3. Perform the deletion using the captured ID
            deletePrintJob(con, jobIDToDelete);

            // 4. Clear the buffer to ensure the next menu loop works correctly
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }
        case 4: { // Search Detailed Info
            // This now serves as a "Global Search" by name
            while (!searchIDsByCustomerName(con));
            // If you still want the specific individual search:
            searchPrintJob(con, readInt("Enter Job ID for details: "));
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }
        case 5:
            return;
        }*/
        std::string nameSearch;
        bool foundAny = false;

        switch (choice) {
        case 1: { // Create Job
            if (availableCustomers > 0) {
                foundAny = false;
                while (true) {
                    std::cout << "Enter Customer Name to find ID (or '0' to exit): ";
                    if (std::cin.peek() == '\n') std::cin.ignore();
                    std::getline(std::cin, nameSearch);

                    if (nameSearch == "0") break; // Return to Print Job menu

                    if (searchIDsByCustomerName(con, nameSearch)) {
                        foundAny = true;
                        break;
                    }
                }

                if (foundAny) {
                    int userID = readInt("Enter User ID to assign job: ");
                    int pageCount = readInt("Enter Page Count: ");
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    if (isInventorySufficient(con, pageCount)) {
                        createPrintJob(con, userID, pageCount, 0.50);
                    }
                }
            }
            break;
        }

        case 2: { // Update Job
            //foundAny = false;
            while (true) {
                std::cout << "Enter Customer Name to find Job IDs (or '0' to exit): ";
                if (std::cin.peek() == '\n') std::cin.ignore();
                std::getline(std::cin, nameSearch);

                if (nameSearch == "0") break;

               /* if (searchIDsByCustomerName(con, nameSearch)) {
                    foundAny = true;
                    break;
                }*/
                if (searchUsersByName(con, nameSearch)) {

                    // Step 3: Select User ID
                    int selectedUserID = readInt("Enter UserID from list above to view jobs: ");

                    // Step 4: Show Jobs for that User
                    if (listJobsForUser(con, selectedUserID)) {

                        // Step 5: Select specific Job ID for full details
                        int jobID = readInt("Enter Job ID from the list above to update: ");
                        int newPages = readInt("New Page Count (0 to skip): ");
                        updatePrintJob(con, jobID, newPages, 0.0);
                    }
                }
            }

            /*if (foundAny) {
                int jobID = readInt("Enter Job ID from the list above to update: ");
                int newPages = readInt("New Page Count (0 to skip): ");
                updatePrintJob(con, jobID, newPages, 0.0);
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }*/
            // Step 2: Show matching Users
            
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            cout << "Press Enter to return to menu...";
            cin.get();
            goto ExitCase2;
            ExitCase2:
            break;
        }

        case 3: { // Delete Job
            //foundAny = false;
            while (true) {
                std::cout << "Enter Customer Name to find Job IDs (or '0' to exit): ";
                if (std::cin.peek() == '\n') std::cin.ignore();
                std::getline(std::cin, nameSearch);

                if (nameSearch == "0") break;
                if (searchUsersByName(con, nameSearch)) {

                    // Step 3: Select User ID
                    int selectedUserID = readInt("Enter UserID from list above to view jobs: ");

                    // Step 4: Show Jobs for that User
                    if (listJobsForUser(con, selectedUserID)) {

                        // Step 5: Select specific Job ID for full details
                        int jobIDToDelete = readInt("Enter Job ID to Delete: ");
                        deletePrintJob(con, jobIDToDelete);
                    }
                }

                /*if (searchIDsByCustomerName(con, nameSearch)) {
                    foundAny = true;
                    break;
                }*/
            }

            //if (foundAny) {
               // int jobIDToDelete = readInt("Enter Job ID to Delete: ");
               // deletePrintJob(con, jobIDToDelete);
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            //}
            break;
        }

        /*case 4: { // Search Detailed Info
            std::cout << "Enter Customer Name to search (or '0' to exit): ";
            if (std::cin.peek() == '\n') std::cin.ignore();
            std::getline(std::cin, nameSearch);

            if (nameSearch != "0") {
                if (searchIDsByCustomerName(con, nameSearch)) {
                    searchPrintJob(con, readInt("Enter Job ID for details: "));
                }
            }
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }*/
        case 4: { // Search Print Job Info

            // Step 1: Get Name
            string nameSearch;
            cout << "Enter Customer Name to search (or '0' to exit): ";
            // Handle buffer from previous cin
            if (cin.peek() == '\n') cin.ignore();
            getline(cin, nameSearch);

            if (nameSearch == "0") break;

            // Step 2: Show matching Users
            if (searchUsersByName(con, nameSearch)) {

                // Step 3: Select User ID
                int selectedUserID = readInt("Enter UserID from list above to view jobs: ");

                // Step 4: Show Jobs for that User
                if (listJobsForUser(con, selectedUserID)) {

                    // Step 5: Select specific Job ID for full details
                    int selectedJobID = readInt("Enter JobID for full details: ");
                    searchPrintJob(con, selectedJobID); // Calls your existing detail function
                }
            }
        

            // Clean up buffer before breaking to menu
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }

        case 5:
            return;
        }

        std::cout << "\nOperation Complete. Press Enter to return to menu...";
        std::cin.get(); // This is the pause that saves your search results!
    }
}