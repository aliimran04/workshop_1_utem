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
void updatePrintJob(sql::Connection* con, int jobID, int newPageCount, double newCostPerPage) {
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
int readCustomers(sql::Connection* con) { // <-- Return type is now int

    int customerCount = 0; // Initialize counter
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("SELECT UserID, FullName, Email FROM user WHERE Role = 'Customer'")
        );
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        // Define column widths
        const int ID_W = 5;
        const int NAME_W = 25;
        const int EMAIL_W = 30;
        const int TOTAL_WIDTH = ID_W + NAME_W + EMAIL_W + 4;

        std::cout << "\n--- Available Customer Users ---\n";
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
        std::cout << "| "
            << std::left << std::setw(ID_W - 2) << "User ID" << " | "
            << std::left << std::setw(NAME_W - 3) << "FullName" << " | "
            << std::left << std::setw(EMAIL_W - 3) << "Email" << " |"
            << std::endl;
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;

        // Iterate through results and count them
        while (res->next()) {
            customerCount++; // Increment counter
            std::cout << "| "
                << std::left << std::setw(ID_W - 2) << res->getInt("UserID") << " | "
                << std::left << std::setw(NAME_W - 3) << res->getString("FullName") << " | "
                << std::left << std::setw(EMAIL_W - 3) << res->getString("Email") << " |"
                << std::endl;
        }
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;

        if (customerCount == 0) {
            std::cout << " No users with the 'Customer' role found.\n";
        }
    }
    catch (sql::SQLException& e) {
        std::cerr << "Error retrieving customers: " << e.what() << std::endl;
    }
    return customerCount; // Return the final count
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
        std::unique_ptr<sql::Statement> stmt(con->createStatement());

        // MODIFIED SQL QUERY: Selects UserID (p.UserID) AND FullName (u.FullName)
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
            "SELECT p.JobID, p.UserID, u.FullName, p.PageCount, p.JobCost "
            "FROM printjob p "
            "JOIN user u ON p.UserID = u.UserID "
            "ORDER BY p.JobID DESC"
        ));

        // Define column widths for the new merged view
        const int JOB_ID_W = 6;
        const int USER_ID_W = 7;     // Added back
        const int NAME_W = 20;       // Adjusted size for FullName
        const int PAGE_W = 12;
        const int COST_W = 10;
        const int TOTAL_WIDTH = JOB_ID_W + USER_ID_W + NAME_W + PAGE_W + COST_W + 6;

        std::cout << "\n--- All Existing Print Jobs (Detailed View) ---\n";
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
        std::cout << "| "
            << std::left << std::setw(JOB_ID_W - 2) << "Job ID" << " | "
            << std::left << std::setw(USER_ID_W - 2) << "User ID" << " | " // Display User ID header
            << std::left << std::setw(NAME_W - 2) << "Customer Name" << " | "
            << std::left << std::setw(PAGE_W - 2) << "Page Count" << " | "
            << std::left << std::setw(COST_W - 2) << "Cost ($)" << " |"
            << std::endl;
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;

        bool found = false;
        while (res->next()) {
            found = true;
            std::cout << "| "
                << std::left << std::setw(JOB_ID_W - 2) << res->getInt("JobID") << " | "
                << std::left << std::setw(USER_ID_W - 2) << res->getInt("UserID") << " | " // Display User ID value
                << std::left << std::setw(NAME_W - 2) << res->getString("FullName") << " | "
                << std::left << std::setw(PAGE_W - 2) << res->getInt("PageCount") << " | "
                << std::left << std::setw(COST_W - 2) << std::fixed << std::setprecision(2) << res->getDouble("JobCost") << " |"
                << std::endl;
        }
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;

        if (!found) {
            std::cout << "No print jobs found in the system.\n";
            PrintJobManagementMenu(con);
        }
    }
    catch (sql::SQLException& e) {
        std::cerr << "Error retrieving print jobs: " << e.what() << std::endl;
    }
}


// --- Main Menu Function (PrintJobManagementMenu) ---

void PrintJobManagementMenu(sql::Connection* con) {
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
}