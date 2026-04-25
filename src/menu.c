#include "menu.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

static void print_menu_header(const char *current_month) {
  printf("\n=== Finance Tracker Menu (%s) ===\n", current_month);
  printf("1) List current month entries\n");
  printf("2) Add entry\n");
  printf("3) Show balance\n");
  printf("4) Delete entry by ID (from list)\n");
  printf("5) Delete all for current month\n");
  printf("6) Totals by category (current month)\n");
  printf("0) Exit\n> ");
}

static void get_current_month(char ym[32]) {
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  snprintf(ym, 32, "%04d-%02d", tm->tm_year + 1900, tm->tm_mon + 1);
}

static int read_line(char *buf, size_t bufSize) {
  if (!fgets(buf, bufSize, stdin)) return -1;
  size_t len = strlen(buf);
  if (len && buf[len - 1] == '\n') buf[len - 1] = '\0';
  return 0;
}

static int prompt_int(const char *prompt, int *outValue) {
  char line[64];
  printf("%s", prompt);
  if (read_line(line, sizeof(line)) != 0) return -1;
  char *end = NULL;
  long v = strtol(line, &end, 10);
  if (end == line || *end != '\0') return -1;
  *outValue = (int)v;
  return 0;
}

static int prompt_double(const char *prompt, double *outValue) {
  char line[128];
  printf("%s", prompt);
  if (read_line(line, sizeof(line)) != 0) return -1;
  char *end = NULL;
  double v = strtod(line, &end);
  if (end == line || *end != '\0') return -1;
  *outValue = v;
  return 0;
}

static int prompt_string(const char *prompt, char *outBuf, size_t outSize) {
  printf("%s", prompt);
  return read_line(outBuf, outSize);
}

// Very small shell escaping to pass arguments safely to system():
// Wraps the string in single quotes and escapes any single quotes inside with '\''
static void shell_escape(const char *in, char *out, size_t outSize) {
  size_t pos = 0;
  if (outSize == 0) return;
  if (pos < outSize) out[pos++] = '\'';
  for (size_t i = 0; in[i] != '\0' && pos + 4 < outSize; ++i) {
    if (in[i] == '\'') {
      // append: '\'' (four chars)
      if (pos + 4 >= outSize) break;
      out[pos++] = '\'';
      out[pos++] = '\\';
      out[pos++] = '\'';
      out[pos++] = '\'';
    } else {
      out[pos++] = in[i];
    }
  }
  if (pos < outSize) out[pos++] = '\'';
  if (pos <= outSize) out[pos] = '\0';
  else out[outSize - 1] = '\0';
}

int run_menu(const char *prog_name, const char *db_file) {
  (void)db_file; // currently unused; main program manages data path
  char ym[32];
  get_current_month(ym);
  while (1) {
    print_menu_header(ym);
    int choice = -1;
    if (prompt_int("", &choice) != 0) { printf("Invalid input.\n"); continue; }
    if (choice == 0) { printf("Bye.\n"); return 0; }
    if (choice == 1) {
      char cmd[256]; snprintf(cmd, sizeof(cmd), "%s filter --month %s", prog_name, ym);
      int rc = system(cmd); (void)rc;
    } else if (choice == 2) {
      char date[16], type[32], category[128], desc[512]; double amount;
      if (prompt_string("Date (YYYY-MM-DD): ", date, sizeof(date)) != 0) { printf("Cancelled.\n"); continue; }
      if (prompt_string("Type (Income/Expense): ", type, sizeof(type)) != 0) { printf("Cancelled.\n"); continue; }
      if (prompt_string("Category: ", category, sizeof(category)) != 0) { printf("Cancelled.\n"); continue; }
      if (prompt_double("Amount: ", &amount) != 0) { printf("Invalid amount.\n"); continue; }
      if (prompt_string("Description: ", desc, sizeof(desc)) != 0) { printf("Cancelled.\n"); continue; }
      
      char escDate[64], escType[128], escCategory[512], escDesc[2048];
      shell_escape(date, escDate, sizeof(escDate));
      shell_escape(type, escType, sizeof(escType));
      shell_escape(category, escCategory, sizeof(escCategory));
      shell_escape(desc, escDesc, sizeof(escDesc));
      
      char cmd[3000];
      snprintf(cmd, sizeof(cmd), "%s add %s %s %s %.2f %s", prog_name, escDate, escType, escCategory, amount, escDesc);
      int rc = system(cmd); (void)rc;
    } else if (choice == 3) {
      char cmd[256]; snprintf(cmd, sizeof(cmd), "%s balance", prog_name); int rc = system(cmd); (void)rc;
    } else if (choice == 4) {
      int id; if (prompt_int("ID to delete: ", &id) != 0) { printf("Invalid ID.\n"); continue; }
      char cmd[256]; snprintf(cmd, sizeof(cmd), "%s delete %d", prog_name, id); int rc = system(cmd); (void)rc;
    } else if (choice == 5) {
      char cmd[256]; snprintf(cmd, sizeof(cmd), "%s delete-month %s", prog_name, ym); int rc = system(cmd); (void)rc;
    } else if (choice == 6) {
      char cmd[256]; snprintf(cmd, sizeof(cmd), "%s totals --by category --month %s", prog_name, ym); int rc = system(cmd); (void)rc;
    } else {
      printf("Invalid option.\n");
    }
  }
}
