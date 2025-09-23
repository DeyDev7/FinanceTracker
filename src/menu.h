#ifndef MENU_H
#define MENU_H

// Run the interactive CLI menu.
// prog_name: program name/path (e.g., argv[0]) used to invoke commands.
// db_file: absolute or relative path to the CSV database file.
// Returns 0 on normal exit; non-zero on error.
int run_menu(const char *prog_name, const char *db_file);

#endif // MENU_H
