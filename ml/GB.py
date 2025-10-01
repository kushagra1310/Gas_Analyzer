import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split, GridSearchCV
from sklearn.preprocessing import LabelEncoder
from sklearn.ensemble import GradientBoostingClassifier
from sklearn.metrics import classification_report, confusion_matrix, accuracy_score
import matplotlib.pyplot as plt
import seaborn as sns

# Load dataset
df = pd.read_csv('Gas_Sensors_Measurements.csv')

# Prepare features and labels
feature_names = ['MQ2', 'MQ3', 'MQ5', 'MQ6', 'MQ7', 'MQ8', 'MQ135']
X = df[feature_names].values
y = df['Gas'].values

# Encode labels
label_encoder = LabelEncoder()
y_encoded = label_encoder.fit_transform(y)

# Split dataset
X_train, X_test, y_train, y_test = train_test_split(X, y_encoded, test_size=0.2, random_state=42, stratify=y_encoded)

# Note: Feature scaling is not necessary for Gradient Boosting models.

# Grid search for optimal hyperparameters
# Note: This can be computationally intensive. A smaller grid is used here for demonstration.
param_grid = {
    'n_estimators': [50, 100, 150],
    'learning_rate': [0.05, 0.1, 0.2],
    'max_depth': [3, 5, 7],
    'subsample': [0.8, 1.0] # Using a subsample < 1.0 leads to Stochastic Gradient Boosting
}

gb = GradientBoostingClassifier(random_state=42)
grid_search = GridSearchCV(estimator=gb, param_grid=param_grid, cv=5, scoring='accuracy', n_jobs=-1, verbose=1)
grid_search.fit(X_train, y_train)

print(f'Best parameters: {grid_search.best_params_}')
print(f'Best cross-validation score: {grid_search.best_score_*100:.2f}%')

# Train with best parameters
best_gb = grid_search.best_estimator_

# Predictions
y_pred = best_gb.predict(X_test)

# Accuracy
accuracy = accuracy_score(y_test, y_pred)
print(f'\nTest Accuracy: {accuracy*100:.2f}%')

# Classification report
print('\nClassification Report:')
print(classification_report(y_test, y_pred, target_names=label_encoder.classes_))

# Confusion matrix
cm = confusion_matrix(y_test, y_pred)
plt.figure(figsize=(8, 6))
sns.heatmap(cm, annot=True, fmt='d', cmap='Purples', 
            xticklabels=label_encoder.classes_, 
            yticklabels=label_encoder.classes_)
plt.title(f'Gradient Boosting Confusion Matrix (Estimators={best_gb.n_estimators})')
plt.ylabel('True Label')
plt.xlabel('Predicted Label')
plt.tight_layout()
plt.savefig('gb_confusion_matrix.png', dpi=300, bbox_inches='tight')
plt.close()

# Plot accuracy vs n_estimators
estimator_values = [20, 50, 80, 100, 120, 150, 200]
accuracies = []

for n_est in estimator_values:
    # Use best parameters found by grid search, but vary n_estimators
    gb_temp = GradientBoostingClassifier(n_estimators=n_est,
                                       learning_rate=best_gb.learning_rate,
                                       max_depth=best_gb.max_depth,
                                       subsample=best_gb.subsample,
                                       random_state=42)
    gb_temp.fit(X_train, y_train)
    accuracies.append(gb_temp.score(X_test, y_test))

plt.figure(figsize=(10, 6))
plt.plot(estimator_values, accuracies, marker='o', color='green', linewidth=2, markersize=8)
plt.title('Gradient Boosting Accuracy vs Number of Estimators')
plt.xlabel('Number of Estimators (Trees)')
plt.ylabel('Accuracy')
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig('gb_estimators.png', dpi=300, bbox_inches='tight')
plt.close()


# Plot feature importances
importances = best_gb.feature_importances_
indices = np.argsort(importances)[::-1]

plt.figure(figsize=(10, 6))
plt.title('Feature Importances')
plt.bar(range(X.shape[1]), importances[indices], align='center', color='orange')
plt.xticks(range(X.shape[1]), [feature_names[i] for i in indices], rotation=45)
plt.xlabel('Features')
plt.ylabel('Importance Score')
plt.tight_layout()
plt.savefig('gb_feature_importances.png', dpi=300, bbox_inches='tight')
plt.close()

print(f'\nGradient Boosting model training completed!')
print(f'Final Test Accuracy: {accuracy*100:.2f}%')