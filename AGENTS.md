# AGENTS.md

Django 6.1 web app (Python 3.12). Single app `transactions`; settings in `config/`.

## Commands
- Use venv: `source venv/bin/activate` (already present).
- Run dev server: `python manage.py runserver`
- Migrate: `python manage.py migrate`
- Tests: `python manage.py test` (Django TestCase; `transactions/tests.py` currently empty)
- Single test: `python manage.py test transactions.tests.TestName`
- Create migration: `python manage.py makemigrations transactions`

## Structure
- `config/` project settings/urls; `transactions/` app (models, views, urls, templates).
- URLs: `config/urls.py` includes `transactions.urls`; root `/` -> `views.dashboard`.
- DB: SQLite `db.sqlite3` (gitignored but present locally); do not commit.

## Conventions
- 4 spaces; snake_case functions/vars.
- Templates under `transactions/templates/transactions/`.
- `Transaction.type` choices are `'Income'`/`'Expense'` (capitalized) — views filter on these exact strings.

## Gotchas
- `transactions/migrations/test_migration.py` is junk (prints hello); not a real migration. Don't run it.
- Uncommitted WIP: dashboard view, urls wiring, sample-data migration 0002. Continue from current state; do not discard.
- `SECRET_KEY` is insecure placeholder; DEBUG=True — dev only.
