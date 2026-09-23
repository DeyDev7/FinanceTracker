# Finance Tracker Web App

This is a Django-based web application for personal finance tracking, migrated from a legacy C CLI tool.

## Migration Plan

See [MIGRATION_PLAN.md](MIGRATION_PLAN.md) for the detailed migration plan.

## Project Structure

- `config/`: Django project settings
- `transactions/`: Django app for managing financial transactions
- `db.sqlite3`: SQLite database (development)
- `requirements.txt`: Python dependencies

## Setup

1. Create a virtual environment: `python -m venv venv`
2. Activate it: `source venv/bin/activate` (Windows: `venv\Scripts\activate`)
3. Install dependencies: `pip install -r requirements.txt`
4. Apply migrations: `python manage.py migrate`
5. Create a superuser: `python manage.py createsuperuser`
6. Run the server: `python manage.py runserver`

## Features

- Dashboard showing income, expenses, savings with visualizations
- Page to manage recurring transactions (subscriptions, services, debts)
- Monthly income/expense comparison with categorization
- HTMX for dynamic updates without full page reloads

## License

This project is for personal use.
