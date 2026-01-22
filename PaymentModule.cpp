#include "PaymentModule.h"
#include "printjob.h"
#include "utils.h" // Assumes readInt(), clearScreen() etc. are here
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <limits>
#include <memory> // For unique_ptr

using namespace std;
using namespace sql;

// ==========================================
// HELPER FUNCTIONS
// ==========================================

// Check if a payment record already exists for a specific JobID
bool checkPaymentExistsForJob(sql::Connection* con, int jobID) {
    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("SELECT 1 FROM payment WHERE JobID = ? LIMIT 1")
        );
        pstmt->setInt(1, jobID);
        unique_ptr<ResultSet> res(pstmt->executeQuery());
        return res->next();
    }
    catch (SQLException& e) {
        cerr << "DB Error (Check Payment): " << e.what() << endl;
        return false;
    }
}



// Check if Job exists AND belongs to User. Returns JobCost if found, -1.0 if not.
double getJobCostIfValid(sql::Connection* con, int jobID, int userID) {
    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("SELECT JobCost FROM printjob WHERE JobID = ? AND UserID = ? LIMIT 1")
        );
        pstmt->setInt(1, jobID);
        pstmt->setInt(2, userID);
        unique_ptr<ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            return res->getDouble("JobCost");
        }
    }
    catch (SQLException& e) {
        cerr << "DB Error (Get Job Cost): " << e.what() << endl;
    }
    return -1.0; // Indicates Invalid Job or User mismatch
}

// Helper to check if User exists (Generic)
bool checkUserIDExists(sql::Connection* con, int uid) {
    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("SELECT 1 FROM user WHERE UserID = ? LIMIT 1")
        );
        pstmt->setInt(1, uid);
        unique_ptr<ResultSet> res(pstmt->executeQuery());
        return res->next();
    }
    catch (SQLException& e) {
        return false;
    }


}
// Helper 1: Search for users by name (Identical logic, reused for Payment context)
bool searchUsersByName1(sql::Connection* con, string nameInput) {
    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("SELECT UserID, FullName, Email FROM user WHERE FullName LIKE ? AND Role = 'Customer'")
        );
        pstmt->setString(1, "%" + nameInput + "%");
        unique_ptr<ResultSet> res(pstmt->executeQuery());

        if (!res->isBeforeFirst()) {
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

// Helper 2: List all PAYMENTS for a specific UserID (With Pagination)
bool listPaymentsForUser(sql::Connection* con, int userID) {
    try {
        // Query the PAYMENT table
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("SELECT TransactionID, Amount, Method, PaymentStatus, TimeStamp FROM payment WHERE UserID = ? ORDER BY TimeStamp DESC")
        );
        pstmt->setInt(1, userID);
        unique_ptr<ResultSet> res(pstmt->executeQuery());

        if (!res->isBeforeFirst()) {
            cout << "No payment history found for User ID: " << userID << endl;
            return false;
        }

        cout << "\n--- Payment History for User ID " << userID << " ---\n";
        // Adjusted column headers for Payment data
        cout << left << setw(10) << "TransID" << setw(12) << "Amount" << setw(10) << "Method" << setw(15) << "Status" << setw(25) << "Date/Time" << endl;
        cout << "----------------------------------------------------------------------------" << endl;

        int rowCount = 0;
        int pageSize = 10;

        while (res->next()) {
            cout << left << setw(10) << res->getInt("TransactionID")
                << "$" << setw(11) << fixed << setprecision(2) << res->getDouble("Amount")
                << setw(10) << res->getString("Method")
                << setw(15) << res->getString("PaymentStatus")
                << setw(25) << res->getString("TimeStamp") << endl;

            rowCount++;

            // --- Pagination Logic ---
            if (rowCount % pageSize == 0) {
                cout << "----------------------------------------------------------------------------" << endl;
                cout << ">>> Showing " << rowCount << " payments. Found your Transaction ID?" << endl;
                cout << ">>> Press [Enter] for older payments, or type 's' to stop listing: ";

                string input;
                getline(cin, input);

                if (!input.empty() && tolower(input[0]) == 's') {
                    cout << ">>> Listing stopped." << endl;
                    break;
                }

                cout << "\n--- Page " << (rowCount / pageSize) + 1 << " ---\n";
                cout << left << setw(10) << "TransID" << setw(12) << "Amount" << setw(10) << "Method" << setw(15) << "Status" << setw(25) << "Date/Time" << endl;
                cout << "----------------------------------------------------------------------------" << endl;
            }
        }
        cout << "----------------------------------------------------------------------------" << endl;
        return true;
    }
    catch (sql::SQLException& e) {
        cerr << "Error listing payments: " << e.what() << endl;
        return false;
    }
}
// Add this helper function above your CreatePayment function
bool listUnpaidJobs(sql::Connection* con, int userID) {
    try {
        // Query: Find jobs for this user that are NOT in the payment table (or not 'Complete')
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT JobID, PageCount, JobCost, TimeStamp "
                "FROM printjob "
                "WHERE UserID = ? "
                "AND JobID NOT IN (SELECT JobID FROM payment WHERE PaymentStatus = 'Complete') "
                "ORDER BY JobID DESC"
            )
        );
        pstmt->setInt(1, userID);
        unique_ptr<ResultSet> res(pstmt->executeQuery());

        // Check if list is empty
        if (!res->isBeforeFirst()) {
            cout << "\n[Info] No unpaid print jobs found for User ID " << userID << ".\n";
            return false;
        }

        cout << "\n--- Unpaid Jobs for User " << userID << " ---\n";
        cout << left << setw(10) << "JobID" << setw(10) << "Pages" << setw(12) << "Cost ($)" << setw(25) << "Date" << endl;
        cout << "--------------------------------------------------------\n";

        while (res->next()) {
            cout << left << setw(10) << res->getInt("JobID")
                << setw(10) << res->getInt("PageCount")
                << fixed << setprecision(2) << setw(12) << res->getDouble("JobCost")
                << setw(25) << res->getString("TimeStamp") << endl;
        }
        cout << "--------------------------------------------------------\n";
        return true;
    }
    catch (sql::SQLException& e) {
        cerr << "Error listing unpaid jobs: " << e.what() << endl;
        return false;
    }
}


// PaymentModule.cpp

void createPayment(sql::Connection* con) {
    cout << "\n--- Create New Payment ---\n";

    int uid = readInt("Enter User ID (UID): ");
    if (!checkUserIDExists(con, uid)) {
        cout << "[Error] User ID " << uid << " does not exist.\n";
        return;
    }
    else {
        (!listUnpaidJobs( con, uid));
    };
    

    int jid = readInt("Enter Job ID (JID): ");
    if (checkPaymentExistsForJob(con, jid)) {
        cout << "[Error] A payment record already exists for Job ID " << jid << ".\n";
        return;
    }

    double jobCost = getJobCostIfValid(con, jid, uid);
    if (jobCost < 0) {
        cout << "[Error] Invalid Job ID or Job does not belong to User " << uid << ".\n";
        return;
    }

    cout << "Total Job Cost: $" << fixed << setprecision(2) << jobCost << endl;
    double amount = 0;
    cout << "Enter Payment Amount: ";
    cin >> amount;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    string method;
    cout << "Enter Payment Method (e.g., Cash, Card): ";
    getline(cin, method);
    double balance = amount - jobCost;
    std::cout << ">>> Payment Accepted. Change Due: $" << fixed << setprecision(2) << balance << endl;
    // Business Logic: Determine status based on cost
    string status = (amount >= jobCost) ? "Complete" : "Insufficient";

    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("INSERT INTO payment (UserID, JobID, Amount, Method, PaymentStatus) VALUES (?, ?, ?, ?, ?)")
        );
        pstmt->setInt(1, uid);
        pstmt->setInt(2, jid);
        pstmt->setDouble(3, amount);
        pstmt->setString(4, method);
        pstmt->setString(5, status);

        pstmt->executeUpdate();
        cout << "[Success] Payment recorded. Status: " << status << "\n";
    }
    catch (SQLException& e) {
        cerr << "SQL Error (Insert Payment): " << e.what() << endl;
    }
}

void readAllPayments(sql::Connection* con) {
    try {
        // 1. Get 64-bit Total Count for the header
        std::unique_ptr<sql::Statement> countStmt(con->createStatement());
        std::unique_ptr<sql::ResultSet> countRes(countStmt->executeQuery("SELECT COUNT(*) FROM payment"));
        long long totalPayments = 0;
        if (countRes->next()) totalPayments = countRes->getInt64(1);

        // 2. Stream all results (Most recent first)
        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
            "SELECT p.TransactionID, p.JobID, p.Amount, p.Method, p.TimeStamp, p.PaymentStatus, "
            "u.UserID, u.FullName "
            "FROM payment p JOIN user u ON p.UserID = u.UserID "
            "ORDER BY p.TimeStamp DESC"
        ));

        const int TRANS_ID_W = 8, USER_ID_W = 8, NAME_W = 22, JOB_ID_W = 8, AMOUNT_W = 12, STATUS_W = 14, DATE_W = 12;
        const int TOTAL_WIDTH = TRANS_ID_W + USER_ID_W + NAME_W + JOB_ID_W + AMOUNT_W + STATUS_W + DATE_W + 8;

        int rowCount = 0;
        int pageSize = 20;

        auto printHeader = [&]() {
            std::cout << "\n--- All Payments (" << totalPayments << " records) ---\n";
            std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
            std::cout << "| " << left << setw(TRANS_ID_W - 2) << "TID" << "| " << setw(USER_ID_W - 2) << "UID"
                << "| " << setw(NAME_W - 2) << "Customer" << "| " << setw(JOB_ID_W - 2) << "JID"
                << "| " << setw(AMOUNT_W - 2) << "Amount" << "| " << setw(STATUS_W - 2) << "Status"
                << "| " << setw(DATE_W - 2) << "Date" << " |" << endl;
            std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
            };

        printHeader();

        while (res->next()) {
            std::cout << "| " << left << setw(TRANS_ID_W - 2) << res->getInt("TransactionID")
                << "| " << setw(USER_ID_W - 2) << res->getInt("UserID")
                << "| " << setw(NAME_W - 2) << res->getString("FullName").substr(0, 18)
                << "| " << setw(JOB_ID_W - 2) << res->getInt("JobID")
                << "| $" << setw(AMOUNT_W - 3) << fixed << setprecision(2) << res->getDouble("Amount")
                << "| " << setw(STATUS_W - 2) << res->getString("PaymentStatus")
                << "| " << res->getString("TimeStamp").substr(0, 10) << " |" << endl;

            rowCount++;

            // Pagination Trigger
            if (rowCount % pageSize == 0 && rowCount < totalPayments) {
                std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
                std::cout << ">>> [" << rowCount << "/" << totalPayments << "] [Enter] more, 'c' clear, 's' stop: ";

                std::string input;
                std::getline(std::cin, input); // No 'ws' here so Enter alone works

                if (!input.empty() && std::tolower(input[0]) == 's') break;
                if (!input.empty() && std::tolower(input[0]) == 'c') {
#ifdef _WIN32
                    system("cls");
#else
                    system("clear");
#endif
                }
                printHeader();
            }
        }
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;

    }
    catch (sql::SQLException& e) {
        std::cerr << "SQL Error: " << e.what() << std::endl;
    }
}

// ==========================================
// CORE MODULE LOGIC
// ==========================================



void updatePayment(sql::Connection* con) {
    cout << "\n--- Update Payment ---\n";
    //readAllPayments(con);
    int transID = readInt("Enter TransactionID: ");

    try {
        // 1. Fetch current data
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("SELECT Amount, Method, JobID FROM payment WHERE TransactionID = ?")
        );
        pstmt->setInt(1, transID);
        unique_ptr<ResultSet> res(pstmt->executeQuery());

        if (!res->next()) {
            cout << "[Error] TransactionID Not Exist.\n";
            return;
        }

        double currentAmount = res->getDouble("Amount");
        string currentMethod = res->getString("Method");
        int jobID = res->getInt("JobID");

        // 2. Get New Values
        double newAmount;
        string newMethod;
        char choice;
        bool changed = false;

        cout << "Current Amount: " << currentAmount << ". Update? (y/n): ";
        cin >> choice;
        if (choice == 'y' || choice == 'Y') {
            cout << "Enter New Amount: ";
            cin >> newAmount;
            changed = true;
        }
        else {
            newAmount = currentAmount;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        cout << "Current Method: " << currentMethod << ". Update? (y/n): ";
        cin >> choice;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        if (choice == 'y' || choice == 'Y') {
            cout << "Enter New Method: ";
            getline(cin, newMethod);
            changed = true;
        }
        else {
            newMethod = currentMethod;
        }

        if (!changed) {
            cout << "No changes made.\n";
            return;
        }

        // 3. Recalculate PaymentStatus if amount changed
        string newPaymentStatus = "Insufficient"; // Default

        // Fetch Job Cost from printjob table to compare against new Amount
        unique_ptr<PreparedStatement> jobStmt(
            con->prepareStatement("SELECT JobCost FROM printjob WHERE JobID = ?")
        );
        jobStmt->setInt(1, jobID);
        unique_ptr<ResultSet> jobRes(jobStmt->executeQuery());

        if (jobRes->next()) {
            double jobCost = jobRes->getDouble("JobCost");
            if (newAmount >= jobCost) {
                newPaymentStatus = "Complete";
            }
        }

        // 4. Update Database
        unique_ptr<PreparedStatement> updateStmt(
            con->prepareStatement(
                "UPDATE payment SET Amount = ?, Method = ?, PaymentStatus = ? WHERE TransactionID = ?"
            )
        );
        updateStmt->setDouble(1, newAmount);
        updateStmt->setString(2, newMethod);
        updateStmt->setString(3, newPaymentStatus);
        updateStmt->setInt(4, transID);

        updateStmt->executeUpdate();
        cout << "[Success] Payment updated. New PaymentStatus: " << newPaymentStatus << "\n";

    }
    catch (SQLException& e) {
        cerr << "[Error] SQL Error: " << e.what() << endl;
    }
}

void deletePayment(sql::Connection* con) {
    cout << "\n--- Delete Payment ---\n";
    //readAllPayments(con);
    int transID = readInt("Enter TransactionID: ");

    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("DELETE FROM payment WHERE TransactionID = ?")
        );
        pstmt->setInt(1, transID);

        int rows = pstmt->executeUpdate();
        if (rows > 0) {
            cout << "[Success] TransactionID Deleted.\n";
        }
        else {
            cout << "[Error] TransactionID Not Exist.\n";
        }
    }
    catch (SQLException& e) {
        cerr << "[Error] SQL Error: " << e.what() << endl;
    }
}

void searchPayment(sql::Connection* con) {
    cout << "\n--- Search Payment ---\n";
    //readAllPayments(con);
    int transID = readInt("Enter TransactionID: ");

    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("SELECT * FROM payment WHERE TransactionID = ?")
        );
        pstmt->setInt(1, transID);
        unique_ptr<ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            cout << "--------------------------------\n";
            cout << "Transaction ID : " << res->getInt("TransactionID") << "\n";
            cout << "User ID        : " << res->getInt("UserID") << "\n";
            cout << "Job ID         : " << res->getInt("JobID") << "\n";
            cout << "Amount         : " << fixed << setprecision(2) << res->getDouble("Amount") << "\n";
            cout << "Method         : " << res->getString("Method") << "\n";
            cout << "Timestamp      : " << res->getString("TimeStamp") << "\n";
            cout << "PaymentStatus         : " << res->getString("PaymentStatus") << "\n";
            cout << "--------------------------------\n";
        }
        else {
            cout << "[Error] TransactionID Not Exist.\n";
        }
    }
    catch (SQLException& e) {
        cerr << "[Error] SQL Error: " << e.what() << endl;
    }
}

// ==========================================
// MENU LOOP
// ==========================================

void runPaymentModule(sql::Connection* con) {
    int choice;
    do {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif

        cout << "\n=====================================\n";
        cout << "    Payment Management Module\n";
        cout << "=====================================\n";
        cout << "1. Create New Payment\n";
        cout << "2. Update Payment Transaction Info\n";
        cout << "3. Delete Payment Transaction\n";
        cout << "4. Search Payment Transaction\n";
        cout << "5. Return to Main Menu\n";

        choice = readInt("Enter your choice (1-5): ");
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        /*switch (choice) {
        case 1: {
            // Loop until a customer with jobs is found
            while (!searchIDsByCustomerName(con));
            createPayment(con);
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }
        case 2: {
            // Search helps find the Transaction ID for updates
            while (!searchIDsByCustomerName(con));
            updatePayment(con);
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }
        case 3: {
            // Search helps verify the correct TID before deletion
            while (!searchIDsByCustomerName(con));
            deletePayment(con);
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }
        case 4: {
            // Simple search to view status
            searchIDsByCustomerName(con);
            searchPayment(con);
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }
        case 5:
            cout << "Exiting Payment Module...\n";
            return;

        default:
            cout << "[Error] Invalid option\n";
            break;
        }

        cout << "\nOperation Complete. Press Enter to continue...";
        cin.get();

    } while (choice != 5);
}*/
        std::string nameSearch;
        switch (choice) {
        case 1: { // Create Payment
            clearScreen();
            while (true) {
                std::cout << "Enter Customer Name to find Job IDs (or '0' to exit): ";
                if (std::cin.peek() == '\n') std::cin.ignore();
                std::getline(std::cin, nameSearch);

                if (nameSearch == "0") break;

                /*if (searchIDsByCustomerName(con, nameSearch)) {
                    createPayment(con);
                    break;
                }*/
                // 1. Find the User
                if (searchUsersByName1(con, nameSearch)) {
                    createPayment(con);

                    // 2. Select the User ID
                   //int selectedUserID = readInt("Enter UserID from list above to view payments: ");

                    // 3. List Payments for that User
                    /*if (listPaymentsForUser(con, selectedUserID)) {

                        // 4. Select specific Transaction ID (Drill down complete)
                        // Assuming you have a detailed search function like searchPaymentDetails(con, id)
                        //int transID = readInt("Enter Transaction ID for full details: ");
                        
                    }*/
                }
            }
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            break;
        } // End Case 1

        case 2: { // Update Payment
            clearScreen();
            while (true) {
                std::cout << "Enter Customer Name to find Transaction IDs (or '0' to exit): ";
                if (std::cin.peek() == '\n') std::cin.ignore();
                std::getline(std::cin, nameSearch);

                if (nameSearch == "0") break;

                /*if (searchIDsByCustomerName(con, nameSearch)) {
                    updatePayment(con);
                    break;
                }*/
                if (searchUsersByName1(con, nameSearch)) {

                    // 2. Select the User ID
                    int selectedUserID = readInt("Enter UserID from list above to view payments: ");

                    // 3. List Payments for that User
                    if (listPaymentsForUser(con, selectedUserID)) {

                        // 4. Select specific Transaction ID (Drill down complete)
                        // Assuming you have a detailed search function like searchPaymentDetails(con, id)
                        //int transID = readInt("Enter Transaction ID for full details: ");
                        updatePayment(con);
                    }
                }
            }
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        } // End Case 2

        case 3: { // Delete Payment
            clearScreen();
            while (true) {
                std::cout << "Enter Customer Name to find Transaction IDs (or '0' to exit): ";
                if (std::cin.peek() == '\n') std::cin.ignore();
                std::getline(std::cin, nameSearch);

                if (nameSearch == "0") break;

                /*if (searchIDsByCustomerName(con, nameSearch)) {
                    deletePayment(con);
                    break;
                }*/
                if (searchUsersByName1(con, nameSearch)) {

                    // 2. Select the User ID
                    int selectedUserID = readInt("Enter UserID from list above to view payments: ");

                    // 3. List Payments for that User
                    if (listPaymentsForUser(con, selectedUserID)) {

                        // 4. Select specific Transaction ID (Drill down complete)
                        // Assuming you have a detailed search function like searchPaymentDetails(con, id)
                        //int transID = readInt("Enter Transaction ID for full details: ");
                        deletePayment(con);
                    }
                }
            }
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        } // End Case 3

        /*case 4: { // Search/View
           //clearScreen();
            std::cout << "Enter Customer Name to search (or '0' to exit): ";
            if (std::cin.peek() == '\n') std::cin.ignore();
            std::getline(std::cin, nameSearch);

            if (nameSearch != "0") {
                if (searchIDsByCustomerName(con, nameSearch)) {
                    searchPayment(con);
                }
            }
            break;
        } // End Case 4*/
        case 4: { // Search Payment Info
            string nameSearch;
            cout << "Enter Customer Name to search (or '0' to exit): ";
            if (cin.peek() == '\n') cin.ignore();
            getline(cin, nameSearch);

            if (nameSearch == "0") break;

            // 1. Find the User
            if (searchUsersByName1(con, nameSearch)) {

                // 2. Select the User ID
                int selectedUserID = readInt("Enter UserID from list above to view payments: ");

                // 3. List Payments for that User
                if (listPaymentsForUser(con, selectedUserID)) {

                    // 4. Select specific Transaction ID (Drill down complete)
                    // Assuming you have a detailed search function like searchPaymentDetails(con, id)
                    //int transID = readInt("Enter Transaction ID for full details: ");
                    searchPayment(con);
                }
            }

            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }

        case 5:
            cout << "Exiting Payment Module...\n";
            return; // Direct return exits the function and loop

        default:
            cout << "[Error] Invalid option\n";
            break;
        } // End switch

        if (choice != 5) {
            cout << "\nOperation Complete. Press Enter to continue...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin.get();
        }

    } while (choice != 5); // End do-while
} // End function body