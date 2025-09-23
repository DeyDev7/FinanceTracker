// Finance tracker CLI
#include "data_model.h" // located in src/data_model.h

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>

static const char *CSV_HEADER = "Date,Type,Category,Amount,Description\n";

static char g_db_dir[64];
static char g_db_file[128];

static void set_db_paths(void) {
  // If current working directory ends with "/src", use "../db"; otherwise use "./db"
  char cwd[PATH_MAX];
  const char *target = "db";
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    size_t len = strlen(cwd);
    if (len >= 4 && strcmp(cwd + (len - 4), "/src") == 0) {
      target = "../db";
    }
  }
  struct stat st;
  if (stat(target, &st) != 0) {
    (void)mkdir(target, 0755);
  }
  snprintf(g_db_dir, sizeof(g_db_dir), "%s", target);
  snprintf(g_db_file, sizeof(g_db_file), "%s/%s", g_db_dir, "data.csv");
}

static int ensure_db_dir_exists(void) {
  struct stat st;
  if (stat(g_db_dir, &st) == 0) return 0; // exists
  if (mkdir(g_db_dir, 0755) == 0) return 0;
  fprintf(stderr, "Error: failed to create '%s' (errno=%d)\n", g_db_dir, errno);
  return -1;
}

static int ensure_csv_has_header(void) {
  FILE *fp = fopen(g_db_file, "r");
  if (!fp) {
    // create new with header
    if (ensure_db_dir_exists() != 0) return -1;
    fp = fopen(g_db_file, "w");
    if (!fp) {
      fprintf(stderr, "Error: cannot open %s for writing\n", g_db_file);
      return -1;
    }
    fputs(CSV_HEADER, fp);
    fclose(fp);
    return 0;
  }
  char firstLine[256];
  if (!fgets(firstLine, sizeof(firstLine), fp)) {
    fclose(fp);
    return 0;
  }
  fclose(fp);
  // naive check; if no header, prepend by rewriting file
  if (strncmp(firstLine, "Date,Type,Category,Amount,Description", 36) != 0) {
    // read entire file then rewrite with header
    fp = fopen(g_db_file, "r");
    if (!fp) return -1;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = (char *)malloc((size_t)size + 1);
    if (!buf) {
      fclose(fp);
      return -1;
    }
    size_t read = fread(buf, 1, (size_t)size, fp);
    buf[read] = '\0';
    fclose(fp);
    fp = fopen(g_db_file, "w");
    if (!fp) { free(buf); return -1; }
    fputs(CSV_HEADER, fp);
    fputs(buf, fp);
    fclose(fp);
    free(buf);
  }
  return 0;
}

static int write_transaction(const Transaction *tx) {
  if (ensure_csv_has_header() != 0) return -1;
  FILE *fp = fopen(g_db_file, "a");
  if (!fp) {
    fprintf(stderr, "Error: cannot open %s for appending\n", g_db_file);
    return -1;
  }
  fprintf(fp, "%s,%s,%s,%.2f,%s\n", tx->date, tx->type, tx->category, tx->amount, tx->description);
  fclose(fp);
  return 0;
}

static int list_transactions(void) {
  if (ensure_csv_has_header() != 0) return -1;
  FILE *fp = fopen(g_db_file, "r");
  if (!fp) { fprintf(stderr, "Error: cannot open %s\n", g_db_file); return -1; }
  char line[512];
  // print header for the list with IDs
  if (fgets(line, sizeof(line), fp)) {
    printf("ID,%s", line); // header already has trailing newline
  }
  int id = 1;
  while (fgets(line, sizeof(line), fp)) {
    printf("%d,%s", id, line);
    id++;
  }
  fclose(fp);
  return 0;
}

static int compute_balance(void) {
  if (ensure_csv_has_header() != 0) return -1;
  FILE *fp = fopen(g_db_file, "r");
  if (!fp) { fprintf(stderr, "Error: cannot open %s\n", g_db_file); return -1; }
  char line[512];
  // skip header if present
  if (fgets(line, sizeof(line), fp)) {
    if (strncmp(line, "Date,Type,Category,Amount,Description", 36) != 0) {
      // first row is data, process it
      // rewind to beginning
      fseek(fp, 0, SEEK_SET);
    }
  }
  double income = 0.0, expense = 0.0;
  while (fgets(line, sizeof(line), fp)) {
    // CSV columns: 0=date,1=type,2=category,3=amount,4=description
    char date[32], type[16], category[64], description[256];
    double amount = 0.0;
    // simple parsing assuming no commas inside fields
    int parsed = sscanf(line, "%31[^,],%15[^,],%63[^,],%lf,%255[^\n]", date, type, category, &amount, description);
    if (parsed >= 4) {
      if (strncmp(type, "Income", 6) == 0) income += amount;
      else expense += amount;
    }
  }
  fclose(fp);
  double balance = income - expense;
  printf("Income: %.2f\n", income);
  printf("Expense: %.2f\n", expense);
  printf("Balance: %.2f\n", balance);
  return 0;
}

// Case-insensitive and typo-tolerant normalization for type field
// Accepts: income/expense (any case), leading letter i/e, or small misspells (edit distance <= 2)
static int min_int(int a, int b) { return a < b ? a : b; }
static int edit_distance_ci(const char *a, const char *b) {
  size_t la = strlen(a), lb = strlen(b);
  int dp[32][32]; // inputs are tiny; guard length
  if (la > 31) la = 31;
  if (lb > 31) lb = 31;
  for (size_t i = 0; i <= la; ++i) dp[i][0] = (int)i;
  for (size_t j = 0; j <= lb; ++j) dp[0][j] = (int)j;
  for (size_t i = 1; i <= la; ++i) {
    for (size_t j = 1; j <= lb; ++j) {
      int ca = tolower((unsigned char)a[i-1]);
      int cb = tolower((unsigned char)b[j-1]);
      int cost = (ca == cb) ? 0 : 1;
      dp[i][j] = min_int(dp[i-1][j] + 1, min_int(dp[i][j-1] + 1, dp[i-1][j-1] + cost));
    }
  }
  return dp[la][lb];
}

static int normalize_type(const char *in, char out[8]) {
  if (!in || !*in) return -1;
  // quick paths
  if (tolower((unsigned char)in[0]) == 'i') { strcpy(out, "Income"); return 0; }
  if (tolower((unsigned char)in[0]) == 'e') { strcpy(out, "Expense"); return 0; }
  // full compares case-insensitive and typo tolerant
  if (edit_distance_ci(in, "income") <= 2) { strcpy(out, "Income"); return 0; }
  if (edit_distance_ci(in, "expense") <= 2) { strcpy(out, "Expense"); return 0; }
  return -1;
}

static int handle_add(int argc, char *argv[]) {
  if (argc < 6) {
    fprintf(stderr, "Usage: %s add YYYY-MM-DD Income|Expense Category Amount Description...\n", argv[0]);
    return 1;
  }
  Transaction tx;
  memset(&tx, 0, sizeof(tx));
  strncpy(tx.date, argv[2], sizeof(tx.date) - 1);
  if (normalize_type(argv[3], tx.type) != 0) {
    fprintf(stderr, "Error: type must be Income or Expense (case-insensitive).\n");
    return 1;
  }
  strncpy(tx.category, argv[4], sizeof(tx.category) - 1);
  tx.amount = atof(argv[5]);
  // join remaining args as description
  size_t descLen = 0;
  tx.description[0] = '\0';
  for (int i = 6; i < argc; ++i) {
    size_t left = sizeof(tx.description) - 1 - descLen;
    if (left == 0) break;
    strncat(tx.description, argv[i], left);
    descLen = strlen(tx.description);
    if (i + 1 < argc && descLen + 1 < sizeof(tx.description)) {
      tx.description[descLen] = ' ';
      tx.description[descLen + 1] = '\0';
      descLen++;
    }
  }
  if (write_transaction(&tx) != 0) return 1;
  printf("Added: %s %s %s %.2f %s\n", tx.date, tx.type, tx.category, tx.amount, tx.description);
  return 0;
}

static int delete_row_by_id(int targetId) {
  if (ensure_csv_has_header() != 0) return -1;
  char tmp_file[160];
  snprintf(tmp_file, sizeof(tmp_file), "%s/%s", g_db_dir, "data.tmp.csv");
  FILE *in = fopen(g_db_file, "r");
  if (!in) { fprintf(stderr, "Error: cannot open %s\n", g_db_file); return -1; }
  FILE *out = fopen(tmp_file, "w");
  if (!out) { fclose(in); fprintf(stderr, "Error: cannot open temp file\n"); return -1; }
  char line[512];
  // copy header
  if (fgets(line, sizeof(line), in)) fputs(line, out);
  int id = 1; int removed = 0;
  while (fgets(line, sizeof(line), in)) {
    if (id == targetId) { removed = 1; /* skip */ }
    else { fputs(line, out); }
    id++;
  }
  fclose(in);
  fclose(out);
  if (!removed) { remove(tmp_file); fprintf(stderr, "Error: ID %d not found.\n", targetId); return 1; }
  if (rename(tmp_file, g_db_file) != 0) { fprintf(stderr, "Error: failed to update database.\n"); return -1; }
  printf("Deleted row ID %d.\n", targetId);
  return 0;
}

static int delete_month(const char *ym) {
  // ym expected as YYYY-MM
  if (!ym || strlen(ym) != 7 || ym[4] != '-') { fprintf(stderr, "Error: month must be YYYY-MM.\n"); return 1; }
  if (ensure_csv_has_header() != 0) return -1;
  char tmp_file[160];
  snprintf(tmp_file, sizeof(tmp_file), "%s/%s", g_db_dir, "data.tmp.csv");
  FILE *in = fopen(g_db_file, "r");
  if (!in) { fprintf(stderr, "Error: cannot open %s\n", g_db_file); return -1; }
  FILE *out = fopen(tmp_file, "w");
  if (!out) { fclose(in); fprintf(stderr, "Error: cannot open temp file\n"); return -1; }
  char line[512];
  if (fgets(line, sizeof(line), in)) fputs(line, out); // header
  int removed = 0;
  while (fgets(line, sizeof(line), in)) {
    char date[16];
    // get first column (date)
    int parsed = sscanf(line, "%15[^,]", date);
    if (parsed == 1 && strncmp(date, ym, 7) == 0) { removed = 1; /* skip */ }
    else { fputs(line, out); }
  }
  fclose(in);
  fclose(out);
  if (!removed) { remove(tmp_file); fprintf(stderr, "No entries found for %s.\n", ym); return 1; }
  if (rename(tmp_file, g_db_file) != 0) { fprintf(stderr, "Error: failed to update database.\n"); return -1; }
  printf("Deleted all entries for %s.\n", ym);
  return 0;
}

static void print_usage(const char *prog) {
  printf("Finance Tracker - Commands and required arguments:\n");
  printf("  %s add YYYY-MM-DD Income|Expense Category Amount Description...\n", prog);
  printf("  %s list\n", prog);
  printf("  %s balance\n", prog);
  printf("  %s delete <ID>\n", prog);
  printf("  %s delete-month YYYY-MM\n\n", prog);
}

int main(int argc, char *argv[]) {
  set_db_paths();
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }
  if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "help") == 0) {
    print_usage(argv[0]);
    return 0;
  }
  if (strcmp(argv[1], "add") == 0) {
    return handle_add(argc, argv);
  } else if (strcmp(argv[1], "list") == 0) {
    return list_transactions();
  } else if (strcmp(argv[1], "balance") == 0) {
    return compute_balance();
  } else if (strcmp(argv[1], "delete") == 0) {
    if (argc < 3) { fprintf(stderr, "Usage: %s delete <ID>\n", argv[0]); return 1; }
    int id = atoi(argv[2]);
    if (id <= 0) { fprintf(stderr, "Error: ID must be a positive integer.\n"); return 1; }
    return delete_row_by_id(id);
  } else if (strcmp(argv[1], "delete-month") == 0) {
    if (argc < 3) { fprintf(stderr, "Usage: %s delete-month YYYY-MM\n", argv[0]); return 1; }
    return delete_month(argv[2]);
  } else {
    fprintf(stderr, "Unknown command: %s\n", argv[1]);
    return 1;
  }
}
