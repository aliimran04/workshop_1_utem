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

            // 1. Get the Role first to determine if a password is needed
            string role = readLineSafe("Role (Admin/Staff/Customer): ");

            string pwd = "N/A"; // Default for Customers

            // 2. Conditional Check: Only ask for password if NOT a customer
            if (role != "Customer" && role != "customer") {
                pwd = readLineSafe("Password: ");
            }
            else {
                cout << "[Note] Password requirement skipped for Customer role.\n";
            }

            // 3. Call your updated createUser function
            createUser(con, name, email, pwd, role);
            break;
        }

        case 2: {
            readUsers(con);
            
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }
        
        /*case 3: { // Update User Info
            clearScreen();
            std::string nameSearch;
            bool found = false;
            std::cout << "=== Update User Management ===\n";

            // 1. Pass "USER" mode to match the new 2-parameter signature
            // This hides JID/TID and avoids the duplicates seen in your logs
            while (!searchIDsByCustomerName(con));

            int id = readInt("Enter UserID to update: ");
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            // 2. Collect the 4 missing strings required by the 6-parameter definition
            std::string n, r, e, p;
            std::cout << "New Name (Leave blank to skip): "; std::getline(std::cin, n);
            std::cout << "New Role (Admin/Customer): "; std::getline(std::cin, r);
            std::cout << "New Email: "; std::getline(std::cin, e);
            std::cout << "New Password: "; std::getline(std::cin, p);

            // 3. Call the function with exactly 6 arguments
            updateUser(con, id, n, r, e, p);
            break;
        }*/
        case 3: { // Update User Info
            clearScreen();
            std::string nameSearch;
            bool foundUsers = false;

            while (true) {
                std::cout << "=== Update User Management ===\n";
                std::cout << "Enter Customer Name to find IDs (or '0' to exit back): ";

                // Ensure the input buffer is clean before reading
                if (std::cin.peek() == '\n') std::cin.ignore();
                std::getline(std::cin, nameSearch);

                // 1. Implementation of the exit back condition
                if (nameSearch == "0") {
                    std::cout << "Returning to main menu...\n";
                    break; // Exits the while(true) loop to hit the final break
                }

                // 2. Call the search function with the input string
                // We modify your previous function to take nameSearch as a parameter
                if (searchIDsByCustomerName(con, nameSearch)) {
                    foundUsers = true;
                    break; // Found results, exit the loop to proceed with the update
                }
                // If not found, the function itself prints the error message
            }

            // Only proceed with updates if we didn't exit via '0' and found a user
            if (foundUsers) {
                int id = readInt("Enter UserID to update: ");
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                std::string n, r, e, p;
                std::cout << "New Name (Leave blank to skip): "; std::getline(std::cin, n);
                std::cout << "New Role (Admin/Customer): "; std::getline(std::cin, r);
                std::cout << "New Email: "; std::getline(std::cin, e);
                std::cout << "New Password: "; std::getline(std::cin, p);

                // 3. Call the function with exactly 6 arguments
                updateUser(con, id, n, r, e, p);
            }
            break; // This breaks the 'case 3' switch statement
        }

        /*case 4: {
            //readUsers(con);
            while (!searchIDsByCustomerName(con));
            
            int id = readInt("Enter UserID to delete: ");
            //cin.ignore(numeric_limits<streamsize>::max(), '\n');

            deleteUser(con, id);
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }*/
        case 4: { // Delete User
            clearScreen();
            std::string nameSearch;
            bool foundUsers = false;

            while (true) {
                std::cout << "=== Delete User Management ===\n";
                std::cout << "Enter Customer Name to find IDs (or '0' to exit back): ";

                // Clean the buffer and get the name
                if (std::cin.peek() == '\n') std::cin.ignore();
                std::getline(std::cin, nameSearch);

                // 1. Sentinel check to return to the previous menu
                if (nameSearch == "0") {
                    std::cout << "Returning to main menu...\n";
                    break;
                }

                // 2. Call search function (updated to 2 parameters)
                if (searchIDsByCustomerName(con, nameSearch)) {
                    foundUsers = true;
                    break;
                }
                // Error message "[Error] No user found..." is handled inside searchIDsByCustomerName
            }

            // 3. Only proceed to delete if we found users and didn't type '0'
            if (foundUsers) {
                int id = readInt("Enter UserID to delete: ");

                // Safety check: Don't delete if UserID is 0
                if (id != 0) {
                    deleteUser(con, id);
                }

                // Clear buffer before returning to menu
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
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
            
            cout << "5. Report Generation\n";
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
            if (role == "Admin") {
                cout << "[Report Generation Module]\n";
                // Call the renamed function from ReportGeneration.h
                runReportGeneration(con);
            }
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