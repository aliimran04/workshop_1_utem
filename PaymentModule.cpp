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

// PaymentModule.cpp

// ... existing code ...

void readAllPayments(sql::Connection* con) {
    try {
        std::unique_ptr<sql::Statement> stmt(con->createStatement());

        // SQL Query: Joins payment (p) and user (u) tables on UserID
        // This query fetches all necessary details for display.
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
            "SELECT p.TransactionID, p.JobID, p.Amount, p.Method, p.TimeStamp, p.PaymentStatus, "
            "u.UserID, u.FullName, u.Email "
            "FROM payment p "
            "JOIN user u ON p.UserID = u.UserID "
            "ORDER BY p.TimeStamp DESC"
        ));

        // Define column widths for the display table
        const int TRANS_ID_W = 6;
        const int USER_ID_W = 7;
        const int NAME_W = 20;
        const int JOB_ID_W = 6;
        const int AMOUNT_W = 10;
        const int PaymentStatus_W = 12;
        const int DATE_W = 20;
        const int TOTAL_WIDTH = TRANS_ID_W + USER_ID_W + NAME_W + JOB_ID_W + AMOUNT_W + PaymentStatus_W + DATE_W + 8;

        std::cout << "\n--- All Payment Transactions and User Details ---\n";
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
        std::cout << "| "
            << std::left << std::setw(TRANS_ID_W - 2) << "TID" << " | "
            << std::left << std::setw(USER_ID_W - 2) << "U ID" << " | "
            << std::left << std::setw(NAME_W - 2) << "Customer Name" << " | "
            << std::left << std::setw(JOB_ID_W - 2) << "J ID" << " | "
            << std::left << std::setw(AMOUNT_W - 2) << "Amount" << " | "
            << std::left << std::setw(PaymentStatus_W - 2) << "PaymentStatus" << " | "
            << std::left << std::setw(DATE_W - 2) << "Date" << " |"
            << std::endl;
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;

        bool found = false;
        while (res->next()) {
            found = true;
            std::cout << "| "
                << std::left << std::setw(TRANS_ID_W - 2) << res->getInt("TransactionID") << " | "
                << std::left << std::setw(USER_ID_W - 2) << res->getInt("UserID") << " | "
                << std::left << std::setw(NAME_W - 2) << res->getString("FullName") << " | "
                << std::left << std::setw(JOB_ID_W - 2) << res->getInt("JobID") << " | "
                << std::left << std::setw(AMOUNT_W - 2) << std::fixed << std::setprecision(2) << res->getDouble("Amount") << " | "
                << std::left << std::setw(PaymentStatus_W - 2) << res->getString("PaymentStatus") << " | "
                << std::left << std::setw(DATE_W - 2) << res->getString("TimeStamp") << " |"
                << std::endl;
        }
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;

        if (!found) {
            std::cout << "No payment transactions found in the system.\n";
        }
    }
    catch (sql::SQLException& e) {
        std::cerr << "Error retrieving all payments: " << e.what() << std::endl;
    }
}

// ==========================================
// CORE MODULE LOGIC
// ==========================================

void createPayment(sql::Connection* con) {
    cout << "\n--- Create New Payment ---\n";
    readPrintJobs(con);

    // 1. Get UID
    int uid = readInt("Enter User ID: ");

    // Check if User exists
    if (!checkUserIDExists(con, uid)) {
        cout << "[Error] User Not Exist.\n";
        return;
    }

    // 2. Get JobID
    int jobId = readInt("Enter Job ID: ");

    // 3. Validate Job Ownership and Get Cost
    double jobCost = getJobCostIfValid(con, jobId, uid);
    if (jobCost < 0) {
        cout << "[Error] Job Not Exist (or does not belong to this UID).\n";
        return;
    }

    // 4. Check for duplicate payment
    if (checkPaymentExistsForJob(con, jobId)) {
        cout << "[Error] Payment already exists for this Job (No partial payments).\n";
        return;
    }

    // 5. Get Payment Details
    double amount;
    cout << "Job Cost is: $" << fixed << setprecision(2) << jobCost << endl;
    cout << "Enter Payment Amount: ";
    while (!(cin >> amount)) {
        cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid amount. Try again: ";
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    string method;
    cout << "Enter Method (e.g., Cash, Card): ";
    getline(cin, method);

    // Note: Database usually handles TimeStamp via DEFAULT CURRENT_TIMESTAMP, 
    // but we can allow manual entry if required by requirements, or skip to use DB time.
    // For this code, we will let the DB handle the timestamp to ensure consistency.

    // 6. Determine PaymentStatus
    string PaymentStatus = (amount >= jobCost) ? "Complete" : "Insufficient";
    // 6.1 Determine Status and Calculate Balance/Change
    string status = (amount >= jobCost) ? "Complete" : "Insufficient";
    double balance = amount - jobCost; // Balance/Change calculation

    // 7. Insert Record
    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement(
                "INSERT INTO payment (UserID, JobID, Amount, Method, PaymentStatus, TimeStamp) VALUES (?, ?, ?, ?, ?, NOW())"
            )
        );
        pstmt->setInt(1, uid);
        pstmt->setInt(2, jobId);
        pstmt->setDouble(3, amount);
        pstmt->setString(4, method);
        pstmt->setString(5, PaymentStatus);

        pstmt->executeUpdate();

        // Get the generated TransactionID
        unique_ptr<Statement> stmt(con->createStatement());
        unique_ptr<ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID() AS id"));
        if (res->next()) {
            cout << "[Success] Payment recorded (" << PaymentStatus << "). Transaction ID: " << res->getInt("id") << "\n";
        }
        // --- ADDED BALANCE DISPLAY LOGIC ---
        if (status == "Complete" && balance > 0.00) {
            cout << "---------------------------------------\n";
            cout << "Amount paid: $" << fixed << setprecision(2) << amount << endl;
            cout << "Job cost:    $" << fixed << setprecision(2) << jobCost << endl;
            cout << "Balance/Change due to customer: $" << fixed << setprecision(2) << balance << endl;
            cout << "---------------------------------------\n";
        }
    }
    catch (SQLException& e) {
        cerr << "[Error] SQL Error: " << e.what() << endl;
    }
}

void updatePayment(sql::Connection* con) {
    cout << "\n--- Update Payment ---\n";
    readAllPayments(con);
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
    readAllPayments(con);
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
    readAllPayments(con);
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
        cout << "\n=====================================\n";
        cout << "   Payment Management Module\n";
        cout << "=====================================\n";
        cout << "1. Create New Payment\n";
        cout << "2. Update Payment Transaction Info\n";
        cout << "3. Delete Payment Transaction\n";
        cout << "4. Search Payment Transaction\n";
        cout << "5. Return to Main Menu\n";

        choice = readInt("Enter your choice (1-5): ");

        switch (choice) {
        case 1: createPayment(con); break;
        case 2: updatePayment(con); break;
        case 3: deletePayment(con); break;
        case 4: searchPayment(con); break;
        case 5: cout << "Exiting Payment Module...\n"; break;
        default: cout << "[Error] Invalid option\n"; break;
        }

    } while (choice != 5);
}