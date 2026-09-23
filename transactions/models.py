from django.db import models

class Transaction(models.Model):
    TYPE_CHOICES = [
        ('Income', 'Income'),
        ('Expense', 'Expense'),
    ]
    date = models.DateField()
    type = models.CharField(max_length=10, choices=TYPE_CHOICES)
    category = models.CharField(max_length=64)
    amount = models.DecimalField(max_digits=10, decimal_places=2)
    description = models.CharField(max_length=256, blank=True)
    recurring = models.BooleanField(default=False)  # For subscriptions/services/debts
    # Optional: frequency for recurring (e.g., monthly, yearly)
    created_at = models.DateTimeField(auto_now_add=True)
    updated_at = models.DateTimeField(auto_now=True)

    def __str__(self):
        return f"{self.date} {self.type}: {self.category} {self.amount}"
