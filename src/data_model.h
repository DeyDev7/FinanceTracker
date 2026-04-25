// Basic shared data model for the finance tracker
#ifndef DATA_MODEL_H
#define DATA_MODEL_H

#include <stdio.h>
#include <string.h>

typedef struct {
  char date[16]; // YYYY-MM-DD + slack
  char type[16]; // "Income" or "Expense"
  char category[64];
  double amount;
  char description[256];
} Transaction;

#endif // DATA_MODEL_H
