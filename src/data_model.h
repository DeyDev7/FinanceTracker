// Basic shared data model for the finance tracker
#ifndef DATA_MODEL_H
#define DATA_MODEL_H

#include <stdio.h>
#include <string.h>

typedef struct {
  char date[11]; // YYYY-MM-DD
  char type[8]; // "Income" or "Expense"
  char category[30];
  double amount;
  char description[100];
} Transaction;

#endif // DATA_MODEL_H
