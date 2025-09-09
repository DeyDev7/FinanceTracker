#include <stdio.h>
#include <string.h>

#ifndef DEBUG
#define MAX_TRANSACTION 10; 


typedef struct {
    char date[11]; // Date for the user to be inputed YYYY-MM-DD
    char type[8]; // type of transaction : "Income" or "Expense" just for the sake of simplicity
    char category[30]; /*Expense = [Rent, Services, Groceries (aka needs),] Income = [Salary, Freelance, SideHustles]*/ 
    double amount; 
    char description[100]; // definition for the user to be insert on the transaction 
} Transaction; 
#endif



