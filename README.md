# CS50 Finance Tracker (CLI)

A command-line finance tracker built for the CS50 course final project.
It records income and expenses to a CSV file and provides basic querying and maintenance commands.

## Features

- Add transactions (Income or Expense)
- List transactions with generated row IDs
- Compute totals and balance
- Delete a transaction by ID
- Delete all transactions for a given month (YYYY-MM)
- Filter by month, category, and type
- Edit a transaction by ID
- Aggregate totals by month or category
 - Simple interactive menu for current month

## Requirements

- GCC or Clang
- Linux/macOS (tested on Linux/WSL)

## Build

From the project root:

```bash
gcc -Wall -Wextra -O2 src/main.c -o finance
```

## Data Storage

- CSV file at `db/data.csv` in the project root. If you run the program from inside `src/`, it still writes to `../db/data.csv`.
- The CSV has a header: `Date,Type,Category,Amount,Description`.

## Usage

```bash
./finance --help
```

Commands:

- Add a transaction
  ```bash
  ./finance add YYYY-MM-DD Income|Expense Category Amount Description...
  ```

- List all transactions (prints an ID column for each row)
  ```bash
  ./finance list
  ```

- Compute totals and balance
  ```bash
  ./finance balance
  ```

- Delete a transaction by ID (use the ID from the list output)
  ```bash
  ./finance delete <ID>
  ```

- Delete all transactions for a given month
  ```bash
  ./finance delete-month YYYY-MM
  ```

- Filter transactions
  ```bash
  ./finance filter [--month YYYY-MM] [--category NAME] [--type Income|Expense]
  ```

- Edit a transaction by ID
  ```bash
  ./finance edit <ID> [--date YYYY-MM-DD] [--type Income|Expense] [--category NAME] [--amount NUM] [--description TEXT...]
  ```

- Aggregate totals
  ```bash
  ./finance totals [--by month|category] [--month YYYY-MM] [--category NAME] [--type Income|Expense]
  ```

- Interactive menu (current month shortcuts)
  ```bash
  ./finance menu
  ```

## Notes

- Transaction type is case-insensitive and tolerant of minor misspellings; inputs are normalized to `Income` or `Expense`.
- Date must be `YYYY-MM-DD`. Amount must be non-negative.
- CSV parsing assumes fields do not contain commas.
- Feel free to extend with filters (by month, category) or more robust CSV handling.
