# Repository Guidelines

## Project Structure & Module Organization
- Source code is located in the `src/` directory.
  - `src/main.c`: Main entry point.
  - `src/menu.c` and `src/menu.h`: Menu handling.
  - `src/data_model.h`: Data structures.
  - `src/core/`: Core financial logic (`finance_core.c`, `finance_core.h`).
  - `src/utils/`: Utility functions (`csv_utils.c`, `csv_utils.h`).
- The `db/` directory likely contains database files.
- Executables `finance` and `finance_tracker` are built in the root.

## Build, Test, and Development Commands
- Build: Use `gcc` to compile source files. Example:
  `gcc src/main.c src/menu.c src/core/finance_core.c src/utils/csv_utils.c -o finance`
- Test: Run `src/test_core.c` (if applicable) or use any existing test framework. Currently, no automated test framework is detected.
- Clean: Remove generated executables with `rm finance finance_tracker`.

## Coding Style & Naming Conventions
- Indentation: 4 spaces.
- Naming: 
  - Functions and variables: snake_case.
  - Macros: UPPER_SNAKE_CASE.
  - Header files: .h extension.
- No automated linting tools are configured; follow the existing code style.

## Testing Guidelines
- Tests are not currently automated. The file `src/test_core.c` may contain test cases.
- To run tests, compile and execute `test_core.c` similarly to the main program.
- Aim to add unit tests for new functions using a testing framework (e.g., Unity, CMocka) in the future.

## Commit & Pull Request Guidelines
- Commit messages: Use imperative mood, e.g., "Add feature X", "Fix bug in Y".
- Pull requests should include:
  - A clear description of changes.
  - Reference to any related issues.
  - Screenshots if UI changes are made.
  - Ensure code follows the existing style and passes any tests.
