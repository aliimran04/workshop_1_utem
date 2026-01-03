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
        pstmt->setString(3, pwd); // TODO: Hash password
        pstmt->setString(4, role);
        pstmt->executeUpdate();
        cout << "Registration Success\n";
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
        unique_ptr<sql::Statement> stmt(con->createStatement());
        unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT * FROM user"));
        // Define column widths. Adjust these values based on your data length.
        const int ID_W = 5;
        const int NAME_W = 25;
        const int EMAIL_W = 30;
        const int ROLE_W = 10;
        const int TOTAL_WIDTH = ID_W + NAME_W + EMAIL_W + ROLE_W + 5;
        // 1. Print the Header Row
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
        std::cout << "| "
            << std::left << std::setw(ID_W - 2) << "ID" << " | "
            << std::left << std::setw(NAME_W - 3) << "FullName" << " | "
            << std::left << std::setw(EMAIL_W - 3) << "Email" << " | "
            << std::left << std::setw(ROLE_W - 2) << "Role" << " |"
            << std::endl;
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
        // 2. Iterate through Results
        while (res->next()) {
            std::cout << "| "
                // Get Int
                << std::left << std::setw(ID_W - 2) << res->getInt("UserID") << " | "
                // Get Strings
                << std::left << std::setw(NAME_W - 3) << res->getString("FullName") << " | "
                << std::left << std::setw(EMAIL_W - 3) << res->getString("Email") << " | "
                << std::left << std::setw(ROLE_W - 2) << res->getString("Role") << " |"
                << std::endl;
        }
        // 3. Print the Footer Separator
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;

    }

    catch (sql::SQLException& e) {
        cout << "Error retrieving users: " << e.what() << endl;
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




void searchUser(sql::Connection* con, const std::string& name) {
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("SELECT * FROM user WHERE FullName LIKE ?")
        );
        pstmt->setString(1, "%" + name + "%");

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        // Column widths
        const int ID_W = 5;
        const int NAME_W = 25;
        const int EMAIL_W = 30;
        const int ROLE_W = 10;
        const int TOTAL_WIDTH = ID_W + NAME_W + EMAIL_W + ROLE_W + 5;

        bool found = false;

        // Header
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;
        std::cout << "| "
            << std::left << std::setw(ID_W - 2) << "ID" << " | "
            << std::left << std::setw(NAME_W - 3) << "FullName" << " | "
            << std::left << std::setw(EMAIL_W - 3) << "Email" << " | "
            << std::left << std::setw(ROLE_W - 2) << "Role" << " |"
            << std::endl;
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;

        // Rows
        while (res->next()) {
            found = true;
            std::cout << "| "
                << std::left << std::setw(ID_W - 2) << res->getInt("UserID") << " | "
                << std::left << std::setw(NAME_W - 3) << res->getString("FullName") << " | "
                << std::left << std::setw(EMAIL_W - 3) << res->getString("Email") << " | "
                << std::left << std::setw(ROLE_W - 2) << res->getString("Role") << " |"
                << std::endl;
        }

        // Footer
        std::cout << "+" << std::string(TOTAL_WIDTH - 2, '-') << "+" << std::endl;

        if (!found) {
            std::cout << "User not found.\n";
        }
    }
    catch (sql::SQLException& e) {
        std::cout << "Error searching user: " << e.what() << std::endl;
    }
}