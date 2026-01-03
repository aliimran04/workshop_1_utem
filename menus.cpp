#include <iostream>
#include <limits>
#include "menus.h"
#include "db.h"
#include "user.h"
#include "printjob.h"
#include "utils.h"   // <<--- THIS FIXES "identifier readInt not defined"
#include "PaymentModule.h"
#include "InventoryManagement.h"
#include "SalesAnalysis.h"
#include "ReportGeneration.h"

using namespace std;

// Helper to safely read a full line after numeric input
string readLineSafe(const string& prompt) {
    cout << prompt;
    string input;
    getline(cin, input);
    return input;
}

// --------------------------------------
// USER MANAGEMENT MENU
// --------------------------------------
void UserManagementMenu(sql::Connection* con) {

    while (true) {
        cout << "\n=========== USER MANAGEMENT ===========\n";
        cout << "1. Register/Create User\n";
        cout << "2. View All Users\n";
        cout << "3. Update User Info\n";
        cout << "4. Delete User\n";
        cout << "5. Search User\n";
        cout << "6. Back to Main Menu\n";
        cout << "========================================\n";

        int choice = readInt("Enter choice: ");
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (choice) {

        case 1: {
            string name = readLineSafe("Full Name: ");
            string email = readLineSafe("Email: ");
            string pwd = readLineSafe("Password: ");
            string role = readLineSafe("Role: ");

            createUser(con, name, email, pwd, role);
            break;
        }

        case 2:
            readUsers(con);
            break;

        case 3: {
            readUsers(con);
            int id = readInt("Enter UserID to update: ");
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            string newName = readLineSafe("New Name: ");
            string newEmail = readLineSafe("New Email: ");
            string newPwd = readLineSafe("New Password: ");
            string newRole = readLineSafe("New Role: ");

            updateUser(con, id, newName, newRole, newEmail, newPwd);
            readUsers(con);
            break;
        }

        case 4: {
            readUsers(con);
            int id = readInt("Enter UserID to delete: ");
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            deleteUser(con, id);
            break;
        }

        case 5: {
            string name = readLineSafe("Search by Full Name: ");
            searchUser(con, name);
            break;
        }

        case 6:
            return;

        default:
            cout << "Invalid choice. Try again.\n";
        }
    }
}

// --------------------------------------
// MAIN MENU (AFTER LOGIN)
// --------------------------------------
void MainMenu(sql::Connection* con) {

    string role;

    // LOGIN LOOP
    while (!login(con, role)) {
        cout << "Login failed. Please try again.\n";
    }

    // MAIN MENU LOOP
    while (true) {

        cout << "\n========== PRINTING SHOP MANAGEMENT SYSTEM MENU ==========\n";

        if (role == "Admin") {
            cout << "1. User Management\n";
        }
        cout << "2. Printing Job Management\n";
        cout << "3. Payment Management\n";
        cout << "4. Inventory Management\n";

        if (role == "Admin") {
            cout << "5. Sales Analysis\n";
            cout << "6. Report Generation\n";
        }

        cout << "7. Logout\n";
        cout << "==========================================\n";

        int choice = readInt("Enter choice: ");
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        // STAFF ACCESS CONTROL
        if (role == "Staff" && (choice == 1 || choice == 5 || choice == 6)) {
            cout << "Access denied. Staff cannot access this feature.\n";
            continue;
        }

        switch (choice) {

        case 1:
            if (role == "Admin")
                UserManagementMenu(con);
            else
                cout << "Access denied.\n";
            break;

        case 2:
            PrintJobManagementMenu(con);
            
            break;

        case 3:
            cout << "[Payment Module]\n";
            runPaymentModule(con);
            break;

        case 4:
            cout << "[Inventory Module]\n";
            runInventoryModule(con);
            break;

        case 5:
            if (role == "Admin")
                cout << "[Sales Analysis Module]\n";
            runSalesAnalysisModule(con);
            break;

        case 6:
            if (role == "Admin")
                cout << "[Report Generation Module]\n";
            runReportGenerationModule(con);
            break;

        case 7:
            cout << "Logging out...\n";
            return;

        default:
            cout << "Invalid choice. Try again.\n";
        }
    }
}

// --------------------------------------
// MAIN PROGRAM LOOP
// --------------------------------------
/*int main() {
    sql::Connection* con = connectDB();

    bool keepRunning = true;

    while (keepRunning) {

        MainMenu(con);  // user logs in + navigates menu

        cout << "\n----------------------------------------\n";
        cout << "You have logged out.\n";
        cout << "1. Login again\n";
        cout << "2. Shutdown system\n";

        int shutdownChoice = readInt("Enter choice: ");
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (shutdownChoice == 2) {
            keepRunning = false;
        }
        else if (shutdownChoice != 1) {
            cout << "Invalid choice. Returning to login.\n";
        }
    }

    delete con;
    cout << "System shutdown complete. Goodbye!\n";
    return 0;
}
*/