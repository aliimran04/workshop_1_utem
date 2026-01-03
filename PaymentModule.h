#pragma once

#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <string>

// ==========================================
// FUNCTION DECLARATIONS
// ==========================================

// Main Menu Entry Point for this module
void runPaymentModule(sql::Connection* con);

// CRUD Operations
void createPayment(sql::Connection* con);
void updatePayment(sql::Connection* con);
void deletePayment(sql::Connection* con);
void searchPayment(sql::Connection* con);

// Helper / Validation Functions
bool checkPaymentExistsForJob(sql::Connection* con, int jobID);
double getJobCostIfValid(sql::Connection* con, int jobID, int userID);
int readCustomers(sql::Connection* con);
// Display / Utility Function
void readAllPayments(sql::Connection* con);