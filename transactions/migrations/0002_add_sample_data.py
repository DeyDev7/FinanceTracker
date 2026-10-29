from django.db import migrations
import datetime

def add_sample_data(apps, schema_editor):
    Transaction = apps.get_model('transactions', 'Transaction')
    Transaction.objects.create(
        date=datetime.date(2026, 9, 20),
        type='Income',
        category='Salary',
        amount=3000.00,
        description='Monthly salary',
        recurring=True
    )
    Transaction.objects.create(
        date=datetime.date(2026, 9, 21),
        type='Expense',
        category='Rent',
        amount=1200.00,
        description='Apartment rent',
        recurring=True
    )
    Transaction.objects.create(
        date=datetime.date(2026, 9, 22),
        type='Expense',
        category='Groceries',
        amount=150.00,
        description='Weekly groceries',
        recurring=False
    )

def remove_sample_data(apps, schema_editor):
    Transaction = apps.get_model('transactions', 'Transaction')
    Transaction.objects.filter(description__in=[
        'Monthly salary',
        'Apartment rent',
        'Weekly groceries'
    ]).delete()

class Migration(migrations.Migration):

    dependencies = [
        ('transactions', '0001_initial'),
    ]
    operations = [
        migrations.RunPython(add_sample_data, remove_sample_data),
    ]
*** End File
