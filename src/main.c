#include "data_model.h" // located on src/data_model.h 

Transaction abc;

int main(int argv, char *argc[]) {

  FILE *fpt; //! Segmentation fault (core dumped) when trying to write to the file

  // This is data just to test the write function
  strcpy(abc.date, "2023-10-01");
  strcpy(abc.type, "Income");
  strcpy(abc.category, "Salary");
  abc.amount = 2500.00;
  strcpy(abc.description, "October Salary");

  // create (or overwrite) the csv file and add the header
  fpt = fopen("db/data.csv", "w+");
  fprintf(fpt, "Date,Type,Category,Amount,Description\n");

  fprintf(fpt, "%s,%s,%s,%.2f,%s\n", abc.date, abc.type, abc.category, abc.amount, abc.description);
  fclose(fpt);

  return 0;
}
