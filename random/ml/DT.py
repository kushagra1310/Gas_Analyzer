import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split, GridSearchCV
from sklearn.preprocessing import LabelEncoder
from sklearn.tree import DecisionTreeClassifier
from sklearn.metrics import classification_report, confusion_matrix, accuracy_score
import matplotlib.pyplot as plt
import seaborn as sns
import joblib

# Load dataset
df = pd.read_excel('Dataset.xlsx')

# Shuffle the dataset
df = df.sample(frac=1, random_state=42).reset_index(drop=True)

# Prepare features and labels
feature_columns = ['MQ-3', 'MQ-136', 'MQ-137', 'Slope_MQ-3', 'Slope_MQ-136', 'Slope_MQ-137']

try:
    X = df[feature_columns].values
    print(f"✅ Successfully loaded {len(feature_columns)} features.")
except KeyError as e:
    print(f"❌ Error: Column not found in Dataset.xlsx. {e}")
    print("Please ensure your Excel file has columns for slopes.")
    exit()

y = df['Gas'].values

# Encode labels
label_encoder = LabelEncoder()
y_encoded = label_encoder.fit_transform(y)

# Split dataset
X_train, X_test, y_train, y_test = train_test_split(X, y_encoded, test_size=0.2, random_state=42, stratify=y_encoded)

# Note: Feature scaling (StandardScaler) is not necessary for Decision Trees as they are not distance-based.

# Grid search for optimal hyperparameters
param_grid = {
    'criterion': ['gini', 'entropy'],
    'max_depth': [5, 10, 15, 20, None],
    'min_samples_split': [2, 5, 10],
    'min_samples_leaf': [1, 2, 4]
}

dt = DecisionTreeClassifier(random_state=42)
grid_search = GridSearchCV(estimator=dt, param_grid=param_grid, cv=5, scoring='accuracy', n_jobs=-1, verbose=1)
grid_search.fit(X_train, y_train)

print(f'Best parameters: {grid_search.best_params_}')
print(f'Best cross-validation score: {grid_search.best_score_*100:.2f}%')

# Train with best parameters
best_dt = grid_search.best_estimator_

# Predictions
y_pred = best_dt.predict(X_test)

# Accuracy
accuracy = accuracy_score(y_test, y_pred)
print(f'\nTest Accuracy: {accuracy*100:.2f}%')

# Classification report
print('\nClassification Report:')
print(classification_report(y_test, y_pred, target_names=label_encoder.classes_))

# Confusion matrix
cm = confusion_matrix(y_test, y_pred)
plt.figure(figsize=(8, 6))
sns.heatmap(cm, annot=True, fmt='d', cmap='Blues', 
            xticklabels=label_encoder.classes_, 
            yticklabels=label_encoder.classes_)
plt.title(f'Decision Tree Confusion Matrix (Max Depth={best_dt.max_depth})')
plt.ylabel('True Label')
plt.xlabel('Predicted Label')
plt.tight_layout()
plt.savefig('dt_confusion_matrix.png', dpi=300, bbox_inches='tight')
plt.close()

# Plot accuracy vs max_depth
depth_values = range(1, 21) # Checking depths from 1 to 20
accuracies = []

for depth in depth_values:
    dt_temp = DecisionTreeClassifier(max_depth=depth, random_state=42)
    dt_temp.fit(X_train, y_train)
    accuracies.append(dt_temp.score(X_test, y_test))

plt.figure(figsize=(10, 6))
plt.plot(depth_values, accuracies, marker='o', color='red', linewidth=2, markersize=8)
plt.title('Decision Tree Accuracy vs Max Depth')
plt.xlabel('Max Depth')
plt.ylabel('Accuracy')
plt.xticks(depth_values)
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig('dt_max_depth.png', dpi=300, bbox_inches='tight')
plt.close()

# Plot feature importances
importances = best_dt.feature_importances_
indices = np.argsort(importances)[::-1]

plt.figure(figsize=(10, 6))
plt.title('Feature Importances')
plt.bar(range(X.shape[1]), importances[indices], align='center')
plt.xticks(range(X.shape[1]), [feature_columns[i] for i in indices], rotation=45)
plt.xlabel('Features')
plt.ylabel('Importance Score')
plt.tight_layout()
plt.savefig('dt_feature_importances.png', dpi=300, bbox_inches='tight')
plt.close()

# Save the trained model and label encoder
joblib.dump(best_dt, 'dt.pkl')
joblib.dump(label_encoder, 'label_encoder_dt.pkl')

print(f'\n✅ Model saved as dt.pkl')
print(f'✅ Label Encoder saved as label_encoder_dt.pkl')
print(f'\nDecision Tree model training completed!')
print(f'Final Test Accuracy: {accuracy*100:.2f}%')