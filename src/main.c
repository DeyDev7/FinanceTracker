// Finance Tracker CLI
//
// A simple command-line finance tracker for the CS50 final project.
// Features:
// - Add income/expense entries with normalized type parsing
// - List entries with generated IDs
// - Compute balance (Income, Expense, Balance)
// - Delete by ID and delete all entries for a given month (YYYY-MM)
// Data is stored in CSV at db/data.csv (project root) with a header row.
#include "data_model.h" // located in src/data_model.h
#include "menu.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>

static const char *CSV_HEADER = "Date,Type,Category,Amount,Description\n";
// Escape a CSV field per RFC4180-lite: wrap in quotes and double any quotes
static void csv_escape(const char *in, char *out, size_t outSize) {
  if (outSize == 0) return;
  size_t pos = 0;
  // Always quote for simplicity
  if (pos < outSize) out[pos++] = '"';
  for (size_t i = 0; in && in[i] != '\0'; ++i) {
    if (pos + 2 >= outSize) break; // ensure space for possible quote and closing quote
    char c = in[i];
    if (c == '"') {
      if (pos + 2 >= outSize) break;
      out[pos++] = '"';
      out[pos++] = '"';
    } else {
      out[pos++] = c;
    }
  }
  if (pos < outSize) out[pos++] = '"';
  if (pos <= outSize) out[pos] = '\0';
  else out[outSize - 1] = '\0';
}

// Parse a CSV line with 5 columns: date,type,category,amount,description
// Handles quoted fields with doubled quotes. Returns 0 on success.
static int csv_parse_5(const char *line,
                       char date[32], char type[16], char category[64],
                       double *amountOut, char description[256]) {
  const char *p = line;
  char *outs[5];
  size_t outSizes[5];
  char amountBuf[64];
  outs[0] = date; outSizes[0] = 32;
  outs[1] = type; outSizes[1] = 16;
  outs[2] = category; outSizes[2] = 64;
  outs[3] = amountBuf; outSizes[3] = sizeof(amountBuf);
  outs[4] = description; outSizes[4] = 256;
  for (int col = 0; col < 5; ++col) {
    size_t pos = 0; int quoted = 0; int done = 0;
    if (*p == '"') { quoted = 1; p++; }
    while (!done && *p) {
      char c = *p++;
      if (quoted) {
        if (c == '"') {
          if (*p == '"') { // escaped quote
            if (pos + 1 < outSizes[col]) outs[col][pos++] = '"';
            p++;
          } else {
            // end quote
            if (*p == ',') { p++; }
            done = 1;
          }
        } else {
          if (pos + 1 < outSizes[col]) outs[col][pos++] = c;
        }
      } else {
        if (c == ',') { done = 1; }
        else if (c == '\n' || c == '\r') { done = 1; }
        else { if (pos + 1 < outSizes[col]) outs[col][pos++] = c; }
      }
    }
    outs[col][pos] = '\0';
    if (!*p) {
      // reached end of line
      for (int k = col + 1; k < 5; ++k) outs[k][0] = '\0';
      break;
    }
  }
  if (!amountOut) return -1;
  *amountOut = atof(amountBuf);
  return 0;
}

// Validate YYYY-MM string (basic range checks only)
static int is_valid_year_month(const char *ym) {
  if (!ym || strlen(ym) != 7 || ym[4] != '-') return 0;
  for (int i = 0; i < 7; ++i) {
    if (i == 4) continue;
    if (!isdigit((unsigned char)ym[i])) return 0;
  }
  int month = (ym[5] - '0') * 10 + (ym[6] - '0');
  if (month < 1 || month > 12) return 0;
  return 1;
}

// Validate YYYY-MM-DD string (basic range checks; no leap-year logic)
static int is_valid_date(const char *ymd) {
  if (!ymd || strlen(ymd) != 10 || ymd[4] != '-' || ymd[7] != '-') return 0;
  for (int i = 0; i < 10; ++i) {
    if (i == 4 || i == 7) continue;
    if (!isdigit((unsigned char)ymd[i])) return 0;
  }
  int month = (ymd[5] - '0') * 10 + (ymd[6] - '0');
  int day = (ymd[8] - '0') * 10 + (ymd[9] - '0');
  if (month < 1 || month > 12) return 0;
  if (day < 1 || day > 31) return 0;
  return 1;
}


static char g_db_dir[64];
static char g_db_file[128];

// Determine and create (if needed) the database directory and CSV file path.
// If running from the project 'src' directory, store data in '../db/data.csv'.
// Otherwise, default to './db/data.csv' in the current working directory.
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

// Ensure the resolved database directory exists; attempt creation if missing.
static int ensure_db_dir_exists(void) {
  struct stat st;
  if (stat(g_db_dir, &st) == 0) return 0; // exists
  if (mkdir(g_db_dir, 0755) == 0) return 0;
  fprintf(stderr, "Error: failed to create '%s' (errno=%d)\n", g_db_dir, errno);
  return -1;
}

// Ensure the CSV file exists and contains the header row; creates/repairs if necessary.
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
    char tmp_file[160];
    snprintf(tmp_file, sizeof(tmp_file), "%s/%s", g_db_dir, "data.header_fix.tmp.csv");
    fp = fopen(g_db_file, "r");
    if (!fp) return -1;
    FILE *out = fopen(tmp_file, "w");
    if (!out) { fclose(fp); return -1; }
    fputs(CSV_HEADER, out);
    char buf[1024];
    while (fgets(buf, sizeof(buf), fp)) {
      fputs(buf, out);
    }
    fclose(fp);
    fclose(out);
    if (rename(tmp_file, g_db_file) != 0) {
      remove(tmp_file);
      return -1;
    }
  }
  return 0;
}

// Append a single transaction to the CSV file (creates header if needed).
static int write_transaction(const Transaction *tx) {
  if (ensure_csv_has_header() != 0) return -1;
  FILE *fp = fopen(g_db_file, "a");
  if (!fp) {
    fprintf(stderr, "Error: cannot open %s for appending\n", g_db_file);
    return -1;
  }
  char escDate[32], escType[32], escCategory[128], escDesc[512];
  csv_escape(tx->date, escDate, sizeof(escDate));
  csv_escape(tx->type, escType, sizeof(escType));
  csv_escape(tx->category, escCategory, sizeof(escCategory));
  csv_escape(tx->description, escDesc, sizeof(escDesc));
  fprintf(fp, "%s,%s,%s,%.2f,%s\n", escDate, escType, escCategory, tx->amount, escDesc);
  fclose(fp);
  return 0;
}

// Print all transactions, prefixing each data row with a 1-based ID column.
static int list_transactions(void) {
  if (ensure_csv_has_header() != 0) return -1;
  FILE *fp = fopen(g_db_file, "r");
  if (!fp) { fprintf(stderr, "Error: cannot open %s\n", g_db_file); return -1; }
  char line[1024];
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

// Scan all transactions and print Income total, Expense total, and Balance.
static int compute_balance(void) {
  if (ensure_csv_has_header() != 0) return -1;
  FILE *fp = fopen(g_db_file, "r");
  if (!fp) { fprintf(stderr, "Error: cannot open %s\n", g_db_file); return -1; }
  char line[1024];
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
    csv_parse_5(line, date, type, category, &amount, description);
    if (type[0] != '\0') {
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

// Utility: return the smaller of two integers.
static int min_int(int a, int b) { return a < b ? a : b; }
// Compute Levenshtein edit distance between strings case-insensitively (bounded for small inputs).
// Case-insensitive and typo-tolerant normalization for type field
// Accepts: income/expense (any case), leading letter i/e, or small misspells (edit distance <= 2)
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

// Normalize free-form 'type' input into canonical "Income" or "Expense" (case-insensitive, typo tolerant).
// Returns 0 on success and writes the normalized value to 'out'; otherwise returns -1.
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

// Handle 'add' command: parse CLI args, normalize type, and persist a new transaction.
static int handle_add(int argc, char *argv[]) {
  if (argc < 6) {
    fprintf(stderr, "Usage: %s add YYYY-MM-DD Income|Expense Category Amount Description...\n", argv[0]);
    return 1;
  }
  Transaction tx;
  memset(&tx, 0, sizeof(tx));
  if (!is_valid_date(argv[2])) {
    fprintf(stderr, "Error: date must be in format YYYY-MM-DD.\n");
    return 1;
  }
  snprintf(tx.date, sizeof(tx.date), "%s", argv[2]);
  if (normalize_type(argv[3], tx.type) != 0) {
    fprintf(stderr, "Error: type must be Income or Expense (case-insensitive).\n");
    return 1;
  }
  snprintf(tx.category, sizeof(tx.category), "%s", argv[4]);
  tx.amount = atof(argv[5]);
  if (tx.amount < 0.0) {
    fprintf(stderr, "Error: amount must be non-negative.\n");
    return 1;
  }
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

// Delete a single transaction by its 1-based ID (as shown by the list command).
// Rewrites the CSV excluding the selected row.
static int delete_row_by_id(int targetId) {
  if (ensure_csv_has_header() != 0) return -1;
  char tmp_file[160];
  snprintf(tmp_file, sizeof(tmp_file), "%s/%s", g_db_dir, "data.tmp.csv");
  FILE *in = fopen(g_db_file, "r");
  if (!in) { fprintf(stderr, "Error: cannot open %s\n", g_db_file); return -1; }
  FILE *out = fopen(tmp_file, "w");
  if (!out) { fclose(in); fprintf(stderr, "Error: cannot open temp file\n"); return -1; }
  char line[1024];
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

// Delete all transactions for a given month (YYYY-MM). Rewrites the CSV filtering that month.
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
  char line[1024];
  if (fgets(line, sizeof(line), in)) fputs(line, out); // header
  int removed = 0;
  while (fgets(line, sizeof(line), in)) {
    char date[32], type[16], category[64], description[256];
    double amount = 0.0;
    csv_parse_5(line, date, type, category, &amount, description);
    if (strncmp(date, ym, 7) == 0) { removed = 1; /* skip */ }
    else { fputs(line, out); }
  }
  fclose(in);
  fclose(out);
  if (!removed) { remove(tmp_file); fprintf(stderr, "No entries found for %s.\n", ym); return 1; }
  if (rename(tmp_file, g_db_file) != 0) { fprintf(stderr, "Error: failed to update database.\n"); return -1; }
  printf("Deleted all entries for %s.\n", ym);
  return 0;
}

// Print help text describing available commands and required arguments.
static void print_usage(const char *prog) {
  printf("Finance Tracker - Commands and required arguments:\n");
  printf("  %s add YYYY-MM-DD Income|Expense Category Amount Description...\n", prog);
  printf("  %s list\n", prog);
  printf("  %s balance\n", prog);
  printf("  %s delete <ID>\n", prog);
  printf("  %s delete-month YYYY-MM\n", prog);
  printf("  %s filter [--month YYYY-MM] [--category NAME] [--type Income|Expense]\n\n", prog);
  printf("  %s edit <ID> [--date YYYY-MM-DD] [--type Income|Expense] [--category NAME] [--amount NUM] [--description TEXT...]\n", prog);
  printf("  %s totals [--by month|category] [--month YYYY-MM] [--category NAME] [--type Income|Expense]\n\n", prog);
  printf("  %s menu  # interactive menu for current month\n\n", prog);
}

// Program entry point: route subcommands and enforce help/usage behavior.
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
    if (!is_valid_year_month(argv[2])) { fprintf(stderr, "Error: month must be YYYY-MM.\n"); return 1; }
    return delete_month(argv[2]);
  } else if (strcmp(argv[1], "filter") == 0) {
    const char *opt_month = NULL;
    const char *opt_category = NULL;
    char opt_type[8] = {0};
    for (int i = 2; i < argc; ++i) {
      if (strcmp(argv[i], "--month") == 0 && i + 1 < argc) {
        opt_month = argv[++i];
      } else if (strcmp(argv[i], "--category") == 0 && i + 1 < argc) {
        opt_category = argv[++i];
      } else if (strcmp(argv[i], "--type") == 0 && i + 1 < argc) {
        if (normalize_type(argv[i+1], opt_type) != 0) { fprintf(stderr, "Error: --type must be Income or Expense.\n"); return 1; }
        i++;
      } else {
        fprintf(stderr, "Usage: %s filter [--month YYYY-MM] [--category NAME] [--type Income|Expense]\n", argv[0]);
        return 1;
      }
    }
    if (opt_month && !is_valid_year_month(opt_month)) { fprintf(stderr, "Error: --month must be YYYY-MM.\n"); return 1; }
    // perform filtering
    if (ensure_csv_has_header() != 0) return 1;
    FILE *fp = fopen(g_db_file, "r");
    if (!fp) { fprintf(stderr, "Error: cannot open %s\n", g_db_file); return 1; }
    char line[1024];
    // print header
    if (fgets(line, sizeof(line), fp)) fputs(line, stdout);
    while (fgets(line, sizeof(line), fp)) {
      char date[32], type[16], category[64], description[256];
      double amount = 0.0;
      csv_parse_5(line, date, type, category, &amount, description);
      if (opt_month && strncmp(date, opt_month, 7) != 0) continue;
      if (opt_category && strcasecmp(category, opt_category) != 0) continue;
      if (opt_type[0] != '\0' && strcasecmp(type, opt_type) != 0) continue;
      fputs(line, stdout);
    }
    fclose(fp);
    return 0;
  } else if (strcmp(argv[1], "edit") == 0) {
    if (argc < 3) { fprintf(stderr, "Usage: %s edit <ID> [--date YYYY-MM-DD] [--type Income|Expense] [--category NAME] [--amount NUM] [--description TEXT...]\n", argv[0]); return 1; }
    int id = atoi(argv[2]);
    if (id <= 0) { fprintf(stderr, "Error: ID must be a positive integer.\n"); return 1; }
    // Parse options
    const char *opt_date = NULL; char opt_type[8] = {0};
    const char *opt_category = NULL; double opt_amount = -1.0; char opt_desc[256] = {0};
    int have_desc = 0;
    for (int i = 3; i < argc; ++i) {
      if (strcmp(argv[i], "--date") == 0 && i + 1 < argc) {
        opt_date = argv[++i];
      } else if (strcmp(argv[i], "--type") == 0 && i + 1 < argc) {
        if (normalize_type(argv[i+1], opt_type) != 0) { fprintf(stderr, "Error: --type must be Income or Expense.\n"); return 1; }
        i++;
      } else if (strcmp(argv[i], "--category") == 0 && i + 1 < argc) {
        opt_category = argv[++i];
      } else if (strcmp(argv[i], "--amount") == 0 && i + 1 < argc) {
        opt_amount = atof(argv[++i]);
      } else if (strcmp(argv[i], "--description") == 0 && i + 1 < argc) {
        // join remaining as description
        opt_desc[0] = '\0';
        size_t descLen = 0;
        for (int j = i + 1; j < argc; ++j) {
          size_t left = sizeof(opt_desc) - 1 - descLen; if (left == 0) break;
          strncat(opt_desc, argv[j], left);
          descLen = strlen(opt_desc);
          if (j + 1 < argc && descLen + 1 < sizeof(opt_desc)) { opt_desc[descLen] = ' '; opt_desc[descLen+1] = '\0'; descLen++; }
        }
        have_desc = 1;
        break;
      } else {
        fprintf(stderr, "Usage: %s edit <ID> [--date YYYY-MM-DD] [--type Income|Expense] [--category NAME] [--amount NUM] [--description TEXT...]\n", argv[0]);
        return 1;
      }
    }
    if (opt_date && !is_valid_date(opt_date)) { fprintf(stderr, "Error: --date must be YYYY-MM-DD.\n"); return 1; }
    if (opt_amount < 0.0 && opt_amount != -1.0) { fprintf(stderr, "Error: --amount must be non-negative.\n"); return 1; }
    if (ensure_csv_has_header() != 0) return 1;
    // Rewrite file with edited row
    char tmp_file[160]; snprintf(tmp_file, sizeof(tmp_file), "%s/%s", g_db_dir, "data.tmp.csv");
    FILE *in = fopen(g_db_file, "r"); if (!in) { fprintf(stderr, "Error: cannot open %s\n", g_db_file); return 1; }
    FILE *out = fopen(tmp_file, "w"); if (!out) { fclose(in); fprintf(stderr, "Error: cannot open temp file\n"); return 1; }
    char line[512]; if (fgets(line, sizeof(line), in)) fputs(line, out); // header
    int cur = 1; int edited = 0;
    while (fgets(line, sizeof(line), in)) {
      if (cur == id) {
        char date[32], type[16], category[64], description[256]; double amount = 0.0;
        if (csv_parse_5(line, date, type, category, &amount, description) == 0) {
          if (opt_date) snprintf(date, sizeof(date), "%s", opt_date);
          if (opt_type[0]) snprintf(type, sizeof(type), "%s", opt_type);
          if (opt_category) snprintf(category, sizeof(category), "%s", opt_category);
          if (opt_amount >= 0.0) amount = opt_amount;
          if (have_desc) { snprintf(description, sizeof(description), "%s", opt_desc); }
          
          char escDate[32], escType[32], escCategory[128], escDesc[512];
          csv_escape(date, escDate, sizeof(escDate));
          csv_escape(type, escType, sizeof(escType));
          csv_escape(category, escCategory, sizeof(escCategory));
          csv_escape(description, escDesc, sizeof(escDesc));
          fprintf(out, "%s,%s,%s,%.2f,%s\n", escDate, escType, escCategory, amount, escDesc);
          edited = 1;
        } else {
          fputs(line, out); // fallback
        }
      } else {
        fputs(line, out);
      }
      cur++;
    }
    fclose(in); fclose(out);
    if (!edited) { remove(tmp_file); fprintf(stderr, "Error: ID %d not found.\n", id); return 1; }
    if (rename(tmp_file, g_db_file) != 0) { fprintf(stderr, "Error: failed to update database.\n"); return 1; }
    printf("Edited row ID %d.\n", id);
    return 0;
  } else if (strcmp(argv[1], "totals") == 0) {
    const char *groupBy = "month"; // default grouping
    const char *opt_month = NULL; const char *opt_category = NULL; char opt_type[8] = {0};
    for (int i = 2; i < argc; ++i) {
      if (strcmp(argv[i], "--by") == 0 && i + 1 < argc) { groupBy = argv[++i]; }
      else if (strcmp(argv[i], "--month") == 0 && i + 1 < argc) { opt_month = argv[++i]; }
      else if (strcmp(argv[i], "--category") == 0 && i + 1 < argc) { opt_category = argv[++i]; }
      else if (strcmp(argv[i], "--type") == 0 && i + 1 < argc) {
        if (normalize_type(argv[i+1], opt_type) != 0) { fprintf(stderr, "Error: --type must be Income or Expense.\n"); return 1; }
        i++;
      } else {
        fprintf(stderr, "Usage: %s totals [--by month|category] [--month YYYY-MM] [--category NAME] [--type Income|Expense]\n", argv[0]);
        return 1;
      }
    }
    if (opt_month && !is_valid_year_month(opt_month)) { fprintf(stderr, "Error: --month must be YYYY-MM.\n"); return 1; }
    int groupByMonth = (strcmp(groupBy, "month") == 0);
    int groupByCategory = (strcmp(groupBy, "category") == 0);
    if (!groupByMonth && !groupByCategory) { fprintf(stderr, "Error: --by must be 'month' or 'category'.\n"); return 1; }
    if (ensure_csv_has_header() != 0) return 1;
    typedef struct { char key[64]; double income; double expense; } Aggregate;
    Aggregate aggs[512]; int aggCount = 0;
    FILE *fp = fopen(g_db_file, "r"); if (!fp) { fprintf(stderr, "Error: cannot open %s\n", g_db_file); return 1; }
    char line[512]; if (fgets(line, sizeof(line), fp)) {/* skip header */}
    while (fgets(line, sizeof(line), fp)) {
      char date[32], type[16], category[64], description[256]; double amount = 0.0; description[0] = '\0';
      if (csv_parse_5(line, date, type, category, &amount, description) != 0) continue;
      if (opt_month && strncmp(date, opt_month, 7) != 0) continue;
      if (opt_category && strcasecmp(category, opt_category) != 0) continue;
      if (opt_type[0] != '\0' && strcasecmp(type, opt_type) != 0) continue;
      char key[64]; key[0] = '\0';
      if (groupByMonth) { memcpy(key, date, 7); key[7] = '\0'; }
      else if (groupByCategory) { snprintf(key, sizeof(key), "%s", category); }
      int idx = -1;
      for (int i = 0; i < aggCount; ++i) { if (strcasecmp(aggs[i].key, key) == 0) { idx = i; break; } }
      if (idx == -1 && aggCount < 512) { idx = aggCount++; snprintf(aggs[idx].key, sizeof(aggs[idx].key), "%s", key); aggs[idx].income = aggs[idx].expense = 0.0; }
      if (idx != -1) {
        if (strncmp(type, "Income", 6) == 0) aggs[idx].income += amount; else aggs[idx].expense += amount;
      }
    }
    fclose(fp);
    printf("Key,Income,Expense,Balance\n");
    for (int i = 0; i < aggCount; ++i) {
      double balance = aggs[i].income - aggs[i].expense;
      printf("%s,%.2f,%.2f,%.2f\n", aggs[i].key, aggs[i].income, aggs[i].expense, balance);
    }
    return 0;
  } else if (strcmp(argv[1], "menu") == 0) {
    return run_menu(argv[0], g_db_file);
  } else {
    fprintf(stderr, "Unknown command: %s\n", argv[1]);
    return 1;
  }
}
