import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split, GridSearchCV
from sklearn.preprocessing import LabelEncoder, StandardScaler
from sklearn.svm import SVC
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

# Standardize features (critical for SVM)
scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

# Grid search for optimal parameters
param_grid = {
    'C': [0.1, 1, 10, 100],
    'gamma': ['scale', 'auto', 0.001, 0.01, 0.1],
    'kernel': ['rbf', 'linear', 'poly']
}

svm = SVC(random_state=42)
grid_search = GridSearchCV(svm, param_grid, cv=5, scoring='accuracy', n_jobs=-1, verbose=1)
grid_search.fit(X_train_scaled, y_train)

print(f'Best parameters: {grid_search.best_params_}')
print(f'Best cross-validation score: {grid_search.best_score_*100:.2f}%')

# Train with best parameters
best_svm = grid_search.best_estimator_

# Predictions
y_pred = best_svm.predict(X_test_scaled)

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
plt.title(f'SVM Confusion Matrix (Kernel={grid_search.best_params_["kernel"]})')
plt.ylabel('True Label')
plt.xlabel('Predicted Label')
plt.tight_layout()
plt.savefig('svm_confusion_matrix.png', dpi=300, bbox_inches='tight')
plt.close()

# Compare different kernels
kernels = ['linear', 'rbf', 'poly']
kernel_accuracies = []

for kernel in kernels:
    svm_temp = SVC(kernel=kernel, C=1.0, random_state=42)
    svm_temp.fit(X_train_scaled, y_train)
    kernel_accuracies.append(svm_temp.score(X_test_scaled, y_test))

plt.figure(figsize=(10, 6))
plt.bar(kernels, kernel_accuracies, color=['#8E44AD', '#3498DB', '#E74C3C'], alpha=0.8)
plt.title('SVM Accuracy Comparison by Kernel Type')
plt.xlabel('Kernel')
plt.ylabel('Accuracy')
plt.ylim([0, 1])
for i, v in enumerate(kernel_accuracies):
    plt.text(i, v + 0.02, f'{v*100:.2f}%', ha='center', fontweight='bold')
plt.grid(True, alpha=0.3, axis='y')
plt.tight_layout()
plt.savefig('svm_kernel_comparison.png', dpi=300, bbox_inches='tight')
plt.close()

# Save the trained model, scaler and label encoder
joblib.dump(best_svm, 'svm.pkl')
joblib.dump(scaler, 'scaler_svm.pkl')
joblib.dump(label_encoder, 'label_encoder_svm.pkl')

print(f'\n✅ Model saved as svm.pkl')
print(f'✅ Scaler saved as scaler_svm.pkl')
print(f'✅ Label Encoder saved as label_encoder_svm.pkl')
print(f'\nSVM model training completed!')
print(f'Final Test Accuracy: {accuracy*100:.2f}%')

