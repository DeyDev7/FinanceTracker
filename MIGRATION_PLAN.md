# Migration Plan: Finance Tracker from C to Django Web App

## Introduction
This plan outlines the migration of the existing C-based CLI finance tracker to a modern web application using Django, Jinja2, and HTMX. The goal is to retain the core functionality while enhancing user experience with a responsive web interface and additional features as requested.

## Goals
1. Migrate transaction data storage from CSV to Django's ORM (SQLite for development, PostgreSQL for production).
2. Implement a dashboard showing income, expenses, savings with visualizations (proportional circles) and a list of recurring subscriptions/services/debts.
3. Provide a page to manage recurring items (subscriptions, services, debts) with description, pay-date, amount.
4. Provide an income/expense comparison page for the current month with categorization and sources.
5. Ensure the codebase is maintainable, orthogonal, and easy to understand for future maintenance.
6. Use HTMX for dynamic updates without full page reloads, enhancing interactivity.

## Approach
- **Pragmatic Migration**: We will port the core logic incrementally, ensuring each step is functional.
- **Orthogonal Design**: Separate concerns into models, views, templates, and utilities. Avoid tight coupling.
- **Maintainability**: Follow Django best practices, use clear naming conventions, and write comprehensive tests.
- **Data Migration**: Existing CSV data will be imported into the new database via a data migration script.

## Data Migration Strategy
- The existing CSV file (`db/data.csv`) contains columns: Date, Type, Category, Amount, Description.
- We will create a Django model `Transaction` with corresponding fields.
- A data migration script will read the CSV and populate the new database.
- For recurring transactions, we will add a `recurring` boolean field and optionally a `frequency` field (to be implemented in later phases).

## Django Project Structure
```
finance_tracker/
├── finance_tracker/          # Project settings
│   ├── __init__.py
│   ├── settings.py
│   ├── urls.py
│   └── wsgi.py
├── transactions/             # Main app for transactions
│   ├── __init__.py
│   ├── models.py
│   ├── views.py
│   ├── urls.py
│   ├── templates/
│   │   └── transactions/
│   │       ├── dashboard.html
│   │       ├── list.html
│   │       ├── add.html
│   │       ├── edit.html
│   │       ├── recurring.html
│   │       └── comparison.html
│   ├── migrations/
│   ├── forms.py
│   └── utils.py
├── db/                       # Original CSV data (to be migrated)
├── static/                   # CSS, JavaScript, images
└── requirements.txt
```

## Models
We propose the following Django models in `transactions/models.py`:

```python
from django.db import models

class Transaction(models.Model):
    TYPE_CHOICES = [
        ('Income', 'Income'),
        ('Expense', 'Expense'),
    ]
    date = models.DateField()
    type = models.CharField(max_length=10, choices=TYPE_CHOICES)
    category = models.CharField(max_length=64)
    amount = models.DecimalField(max_length=10, decimal_places=2)
    description = models.CharField(max_length=256, blank=True)
    recurring = models.BooleanField(default=False)  # For subscriptions/services/debts
    # Optional: frequency for recurring (e.g., monthly, yearly)
    created_at = models.DateTimeField(auto_now_add=True)
    updated_at = models.DateTimeField(auto_now=True)

    def __str__(self):
        return f"{self.date} {self.type}: {self.category} {self.amount}"
```

## Views and Templates
### Dashboard View
- Shows summary cards: total income, total expenses, net balance.
- A chart (using a JavaScript library like Chart.js or simple CSS circles) for proportional representation of income vs expenses vs savings.
- A table listing recurring transactions (where `recurring=True`) with date, amount, description.

### Recurring Transactions Page
- List all transactions with `recurring=True`.
- Ability to add, edit, delete recurring transactions.
- Form includes: date (start date), type, category, amount, description, and optionally recurrence frequency.

### Income/Expense Comparison Page
- For the selected month (default current month):
  - Income: breakdown by source (category) as a bar chart.
  - Expenses: breakdown by category as a bar chart or pie chart.
  - Table of all transactions for the month with optional description, category (dropdown), date, amount.
  - Ability to add/edit/delete transactions for the month.

## HTMX Integration
- Use HTMX for:
  - Adding a transaction via a form that returns a partial table row to append.
  - Editing a transaction in-place.
  - Deleting a transaction with a confirmation and removal from the DOM.
  - Filtering transactions by month/category without full page reload.
- This avoids full page refreshes and provides a snappy user experience.

## URL Routing
Define URLs in `transactions/urls.py`:
- `dashboard/` -> dashboard view
- `recurring/` -> recurring transactions list
- `recurring/add/` -> add recurring transaction
- `recurring/edit/<int:pk>/` -> edit recurring transaction
- `recurring/delete/<int:pk>/` -> delete recurring transaction
- `comparison/` -> income/expense comparison page
- `transactions/` -> list all transactions (optional)
- `transactions/add/` -> add transaction
- `transactions/edit/<int:pk>/` -> edit transaction
- `transactions/delete/<int:pk>/` -> delete transaction

## Forms and Validation
- Use Django forms for validation (e.g., ensuring date is valid, amount is positive).
- Reuse validation logic from the C CSV utils (date validation, type normalization) where applicable.

## Authentication
- For personal use, authentication may not be strictly necessary. However, we can add a simple login system to secure the data.
- Alternatively, we can rely on network security (private instance) and skip authentication for simplicity.

## Testing
- Write unit tests for models and forms.
- Write integration tests for views using Django's test client.
- Consider using Selenium or Playwright for HTMX interactions, but given the scope, we may start with unit and view tests.

## Deployment Considerations
- Use Gunicorn or uWSGI to serve the Django app.
- Use Nginx as a reverse proxy.
- Set up environment variables for secret keys and database credentials.
- Ensure proper logging and error handling.

## Step-by-Step Migration Steps
1. Set up Django project and app.
2. Define the Transaction model and run migrations.
3. Create a data import script to migrate existing CSV data.
4. Implement the dashboard view and template.
5. Implement recurring transactions CRUD.
6. Implement income/expense comparison view.
7. Add HTMX enhancements for dynamic updates.
8. Add forms and validation.
9. Write tests.
10. Deploy to a test environment and verify with existing data.
11. Refine UI/UX based on feedback.

## Risks and Mitigation
- **Data Loss**: Ensure backup of existing CSV before migration. Test import script thoroughly.
- **Schema Changes**: Use Django migrations to evolve the model safely.
- **UI Complexity**: Start with simple templates and gradually enhance with HTMX.
- **Performance**: Use database indexing on date and category fields for faster queries.

## Conclusion
This migration plan provides a pragmatic path to transform the C CLI tool into a maintainable, feature-rich web app. By leveraging Django's strengths and HTMX for interactivity, we will achieve a modern finance tracker that meets the user's evolving needs.
*** End File
