#include "db.h"
#include "menus.h"
#include "utils.h"


int main() {
    sql::Connection* con = connectDB();

    while (true) {
        MainMenu(con);
        int choice = readInt("\n1. Login again\n2. Exit\n");
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice == 2)
            break;
    }

    delete con;
    return 0;
}
