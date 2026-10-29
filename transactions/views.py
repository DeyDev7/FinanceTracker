from django.shortcuts import render
from django.db.models import Sum
from .models import Transaction

def dashboard(request):
    # Get all transactions
    transactions = Transaction.objects.all()
    
    # Calculate totals
    total_income = transactions.filter(type='Income').aggregate(Sum('amount'))['amount__sum'] or 0
    total_expense = transactions.filter(type='Expense').aggregate(Sum('amount'))['amount__sum'] or 0
    balance = total_income - total_expense
    
    # Get recurring transactions (subscriptions/services/debts)
    recurring_transactions = transactions.filter(recurring=True).order_by('date')
    
    context = {
        'total_income': total_income,
        'total_expense': total_expense,
        'balance': balance,
        'recurring_transactions': recurring_transactions,
    }
    return render(request, 'transactions/dashboard.html', context)
