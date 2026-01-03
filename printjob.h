#pragma once

#include <string>
#include <memory>
#include <cppconn/connection.h>

// Forward declaration of the Print Job Management Menu function
void PrintJobManagementMenu(sql::Connection* con);

// --- CRUD Function Declarations ---

// Create Print Job (based on C1 -> CX3 in flowchart)
void createPrintJob(sql::Connection* con, int userID, int pageCount, double costPerPage);

// Read/Search Print Job (based on S1 -> S3 in flowchart)
void searchPrintJob(sql::Connection* con, int jobID);

// Update Print Job (based on U1 -> UX3 in flowchart)
void updatePrintJob(sql::Connection* con, int jobID, int newPageCount, double newCostPerPage);

// Delete Print Job (based on DEL1 -> DX2 in flowchart)
void deletePrintJob(sql::Connection* con, int jobID);

// Helper function to check if a UserID exists (C2 in flowchart)
bool doesUserExist(sql::Connection* con, int userID);

// Helper function to check if a JobID exists (U2, DEL2, S2 in flowchart)
bool doesJobExist(sql::Connection* con, int jobID);

// Helper function to check inventory (C4 in flowchart)
bool isInventorySufficient(sql::Connection* con, int pageCount);

// Function to display only users with the 'Customer' role and return the count
int countCustomerUsers(sql::Connection* con);
int readCustomers(sql::Connection* con);
// Function to display all available print jobs
void readPrintJobs(sql::Connection* con);