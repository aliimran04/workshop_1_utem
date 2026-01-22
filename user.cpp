#include "user.h"        // <-- VERY IMPORTANT
#include "utils.h"       // for isValidEmail(), isValidRole()
#include <iostream>
#include <iomanip>
#include <memory>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>

using namespace std;
void createUser(sql::Connection* con, const string& fullName, const string& email, const string& pwd, const string& role) {
    try {
        unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("INSERT INTO `user` (FullName, Email, Password, Role) VALUES (?, ?, ?, ?)")
        );

        pstmt->setString(1, fullName);
        pstmt->setString(2, email);

        // Logic Change: Check if the role is Customer
        if (role == "Customer" || role == "customer") {
            pstmt->setString(3, "N/A"); // Default placeholder for customers
        }
        else {
            pstmt->setString(3, pwd);   // Standard password for Admin/Staff
        }

        pstmt->setString(4, role);
        pstmt->executeUpdate();

        if (role == "Customer") {
            cout << "Registration Success (Customer: No password required)\n";
        }
        else {
            cout << "Registration Success\n";
        }
    }
    catch (const sql::SQLException& e) {
        if (e.getErrorCode() == 1062) {
            cout << "Duplicate Registration\n";
        }
        else {
            cout << "Error: " << e.what() << '\n';
        }
    }
}


void readUsers(sql::Connection* con) {
    try {
        // Step 1: Get Total Count for Perspective
        std::unique_ptr<sql::Statement> countStmt(con->createStatement());
        std::unique_ptr<sql::ResultSet> countRes(countStmt->executeQuery("SELECT COUNT(*) FROM user"));
        long long totalUsers = 0;
        if (countRes->next()) totalUsers = countRes->getInt64(1);

        // Step 2: Query for data
        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT UserID, FullName, Email, Role FROM user ORDER BY UserID ASC"));

        const int ID_W = 8, NAME_W = 25, EMAIL_W = 35, ROLE_W = 12;
        const int TOTAL_WIDTH = ID_W + NAME_W + EMAIL_W + ROLE_W + 5;

        int rowCount = 0;
        int pageSize = 20;

        auto printHeader = [&]() {
            std::cout << "\nTotal Registered Users: " << totalUsers << "\n";
            std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
            std::cout << "| " << std::left << std::setw(ID_W - 2) << "ID" << " | "
                << std::left << std::setw(NAME_W - 3) << "FullName" << " | "
                << std::left << std::setw(EMAIL_W - 3) << "Email" << " | "
                << std::left << std::setw(ROLE_W - 2) << "Role" << " |" << std::endl;
            std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
            };

        printHeader();

        while (res->next()) {
            std::cout << "| " << std::left << std::setw(ID_W - 2) << res->getInt("UserID") << " | "
                << std::left << std::setw(NAME_W - 3) << res->getString("FullName") << " | "
                << std::left << std::setw(EMAIL_W - 3) << res->getString("Email") << " | "
                << std::left << std::setw(ROLE_W - 2) << res->getString("Role") << " |" << std::endl;

            rowCount++;

            // Pagination Trigger
            if (rowCount % pageSize == 0) {
                std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
                std::cout << ">>> Page " << (rowCount / pageSize) << " | " << rowCount << " of " << totalUsers << " displayed." << std::endl;
                std::cout << ">>> [Enter] for more, [q] to stop, [c] to clear & continue: ";

                std::string input;
                std::getline(std::cin >> std::ws, input);

                if (!input.empty() && std::tolower(input[0]) == 'q') return;

                if (!input.empty() && std::tolower(input[0]) == 'c') {
#ifdef _WIN32
                    system("cls");
#else
                    system("clear");
#endif
                }
                printHeader(); // Reprint headers for readability
            }
        }
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
        std::cout << "End of User List.\n";

    }
    catch (sql::SQLException& e) {
        std::cout << "Error retrieving users: " << e.what() << std::endl;
    }
}

void updateUser(sql::Connection* con,
    int userID,
    const std::string& newName,
    const std::string& newRole,
    const std::string& newEmail,
    const std::string& newPwd)
{
    try {
        // Check user exists
        {
            std::unique_ptr<sql::PreparedStatement> chk(
                con->prepareStatement("SELECT 1 FROM `user` WHERE UserID=? LIMIT 1"));
            chk->setInt(1, userID);
            std::unique_ptr<sql::ResultSet> rs(chk->executeQuery());
            if (!rs->next()) {
                std::cout << "User Not Exist\n";
                return;
            }
        }

        // Validate inputs (only if provided)
        if (!isValidEmail(newEmail)) {
            std::cout << "Invalid email format.\n";
            return;
        }
        if (!isValidRole(newRole)) {
            std::cout << "Invalid Role\n";
            return;
        }

        // Build dynamic SQL with only non-empty fields
        std::string sql = "UPDATE `user` SET ";
        int setCount = 0;
        if (!newName.empty()) { if (setCount++) sql += ", "; sql += "FullName=?"; }         // or FullName
        if (!newEmail.empty()) { if (setCount++) sql += ", "; sql += "Email=?"; }
        if (!newPwd.empty()) { if (setCount++) sql += ", "; sql += "Password=?"; }
        if (!newRole.empty()) { if (setCount++) sql += ", "; sql += "Role=?"; }
        sql += " WHERE UserID=?";

        if (setCount == 0) {
            std::cout << "No new data. No changes made.\n";
            return;
        }

        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(sql));
        int idx = 1;
        if (!newName.empty())  pstmt->setString(idx++, newName);
        if (!newEmail.empty()) pstmt->setString(idx++, newEmail);
        if (!newPwd.empty())   pstmt->setString(idx++, newPwd); // TODO: hash in production
        if (!newRole.empty())  pstmt->setString(idx++, newRole);
        pstmt->setInt(idx++, userID);

        int affected = pstmt->executeUpdate();
        if (affected == 0) {
            std::cout << "No new data. No changes made.\n";
        }
        else {
            std::cout << "User Updated Successfully\n";
        }

    }
    catch (const sql::SQLException& e) {
        if (e.getErrorCode() == 1062) {
            std::cout << "Duplicate Registration\n"; // duplicate email
        }
        else {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
}

void deleteUser(sql::Connection* con, int userID) {
    try {
        unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("DELETE FROM user WHERE UserID = ?")
        );
        pstmt->setInt(1, userID);
        pstmt->executeUpdate();
        cout << "User deleted successfully!\n";
    }
    catch (sql::SQLException& e) {
        cout << "Error deleting user: " << e.what() << endl;
    }
}

void viewUserDetails(sql::Connection* con, int userID) {
    try {
        unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("SELECT * FROM user WHERE UserID = ?")
        );
        pstmt->setInt(1, userID);
        unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            cout << "\n========================================\n";
            cout << "          USER PROFILE DETAILS          \n";
            cout << "========================================\n";
            cout << "User ID   : " << res->getInt("UserID") << endl;
            cout << "Full Name : " << res->getString("FullName") << endl;
            cout << "Email     : " << res->getString("Email") << endl;
            cout << "Password  : [HIDDEN] " << endl; // Security best practice
            cout << "Role      : " << res->getString("Role") << endl;

            // If you have a CreatedAt column, you can display it too:
            // cout << "Created   : " << res->getString("CreatedAt") << endl;

            cout << "========================================\n";
        }
        else {
            cout << "[Error] User ID " << userID << " not found.\n";
        }
    }
    catch (sql::SQLException& e) {
        cerr << "Error fetching user details: " << e.what() << endl;
    }
}




void searchUser(sql::Connection* con, const std::string& name) {
    try {
        // LIMIT 100 ensures the terminal doesn't crash if the search term is too broad
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("SELECT UserID, FullName, Email, Role FROM user WHERE FullName LIKE ? LIMIT 100")
        );
        pstmt->setString(1, "%" + name + "%");

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        const int ID_W = 8, NAME_W = 25, EMAIL_W = 35, ROLE_W = 12;
        const int TOTAL_WIDTH = ID_W + NAME_W + EMAIL_W + ROLE_W + 5;

        bool found = false;
        std::cout << "\n--- Search Results for '" << name << "' (Max 100) ---\n";

        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
        while (res->next()) {
            if (!found) { // Print header only if first result is found
                std::cout << "| " << std::left << std::setw(ID_W - 2) << "ID" << " | "
                    << std::left << std::setw(NAME_W - 3) << "FullName" << " | "
                    << std::left << std::setw(EMAIL_W - 3) << "Email" << " | "
                    << std::left << std::setw(ROLE_W - 2) << "Role" << " |" << std::endl;
                std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
            }
            found = true;
            std::cout << "| " << std::left << std::setw(ID_W - 2) << res->getInt("UserID") << " | "
                << std::left << std::setw(NAME_W - 3) << res->getString("FullName") << " | "
                << std::left << std::setw(EMAIL_W - 3) << res->getString("Email") << " | "
                << std::left << std::setw(ROLE_W - 2) << res->getString("Role") << " |" << std::endl;
        }
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;

        if (!found) std::cout << "No users found matching that name.\n";

        cout << "\nEnter User ID to view full profile (or 0 to return): ";
        int targetID;
        while (!(cin >> targetID)) {
            cout << "Invalid input. Enter User ID (0 to return): ";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clean buffer

        if (targetID != 0) {
            viewUserDetails(con, targetID);

            // Optional: Pause so they can read it
            cout << "\nPress Enter to return to menu...";
            cin.get();
        }

    }
    catch (sql::SQLException& e) {
        std::cout << "Error searching user: " << e.what() << std::endl;
    }
}