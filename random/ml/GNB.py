import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split, GridSearchCV
from sklearn.preprocessing import LabelEncoder, StandardScaler
from sklearn.naive_bayes import GaussianNB
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

# Standardize features (important for Gaussian Naive Bayes)
scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

# Grid search for optimal var_smoothing
# var_smoothing is a stability parameter added to the variance of each feature
param_grid = {
    'var_smoothing': np.logspace(0, -9, num=100) # Test a range of small values
}

gnb = GaussianNB()
grid_search = GridSearchCV(estimator=gnb, param_grid=param_grid, cv=5, scoring='accuracy', n_jobs=-1, verbose=1)
grid_search.fit(X_train_scaled, y_train)

print(f'Best parameters: {grid_search.best_params_}')
print(f'Best cross-validation score: {grid_search.best_score_*100:.2f}%')

# Use the best estimator found by GridSearchCV
best_gnb = grid_search.best_estimator_

# Predictions
y_pred = best_gnb.predict(X_test_scaled)

# Accuracy
accuracy = accuracy_score(y_test, y_pred)
print(f'\nTest Accuracy: {accuracy*100:.2f}%')

# Classification report
print('\nClassification Report:')
print(classification_report(y_test, y_pred, target_names=label_encoder.classes_))

# Confusion matrix
cm = confusion_matrix(y_test, y_pred)
plt.figure(figsize=(8, 6))
sns.heatmap(cm, annot=True, fmt='d', cmap='YlGnBu', 
            xticklabels=label_encoder.classes_, 
            yticklabels=label_encoder.classes_)
plt.title(f'Gaussian Naive Bayes Confusion Matrix')
plt.ylabel('True Label')
plt.xlabel('Predicted Label')
plt.tight_layout()
plt.savefig('gnb_confusion_matrix.png', dpi=300, bbox_inches='tight')
plt.close()

# Plot CV score vs var_smoothing
# Note: Naive Bayes doesn't have a feature_importances_ attribute like tree models
results = grid_search.cv_results_
plt.figure(figsize=(10, 6))
plt.semilogx(results['param_var_smoothing'].data, results['mean_test_score'], marker='o', linestyle='-')
plt.title('Naive Bayes CV Score vs. Var Smoothing')
plt.xlabel('var_smoothing')
plt.ylabel('Cross-Validation Accuracy')
plt.grid(True)
plt.tight_layout()
plt.savefig('gnb_var_smoothing.png', dpi=300, bbox_inches='tight')
plt.close()

# Save the trained model, scaler and label encoder
joblib.dump(best_gnb, 'gnb.pkl')
joblib.dump(scaler, 'scaler_gnb.pkl')
joblib.dump(label_encoder, 'label_encoder_gnb.pkl')

print(f'\n✅ Model saved as gnb.pkl')
print(f'✅ Scaler saved as scaler_gnb.pkl')
print(f'✅ Label Encoder saved as label_encoder_gnb.pkl')
print(f'\nGaussian Naive Bayes model training completed!')
print(f'Final Test Accuracy: {accuracy*100:.2f}%')