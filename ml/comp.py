import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import LabelEncoder, StandardScaler
from sklearn.neighbors import KNeighborsClassifier
from sklearn.ensemble import RandomForestClassifier
from sklearn.svm import SVC
from sklearn.metrics import accuracy_score, classification_report
import matplotlib.pyplot as plt
import seaborn as sns

# Load dataset
df = pd.read_csv('Gas_Sensors_Measurements.csv')

# Prepare features and labels
X = df[['MQ2', 'MQ3', 'MQ5', 'MQ6', 'MQ7', 'MQ8', 'MQ135']].values
y = df['Gas'].values

# Encode labels
label_encoder = LabelEncoder()
y_encoded = label_encoder.fit_transform(y)

# Split dataset
X_train, X_test, y_train, y_test = train_test_split(X, y_encoded, test_size=0.2, random_state=42, stratify=y_encoded)

# Standardize features
scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

# Initialize models
models = {
    'KNN': KNeighborsClassifier(n_neighbors=5),
    'Random Forest': RandomForestClassifier(n_estimators=200, random_state=42),
    'SVM': SVC(kernel='rbf', C=10, gamma='scale', random_state=42)
}

# Train and evaluate
results = {}
for name, model in models.items():
    print(f'\nTraining {name}...')
    model.fit(X_train_scaled, y_train)
    y_pred = model.predict(X_test_scaled)
    accuracy = accuracy_score(y_test, y_pred)
    results[name] = accuracy
    print(f'{name} Accuracy: {accuracy*100:.2f}%')
    print(classification_report(y_test, y_pred, target_names=label_encoder.classes_))

# Plot comparison
plt.figure(figsize=(10, 6))
models_list = list(results.keys())
accuracies = list(results.values())
colors = ['#3498DB', '#2ECC71', '#E74C3C']

bars = plt.bar(models_list, accuracies, color=colors, alpha=0.8)
plt.title('Model Comparison: Gas Detection Accuracy', fontsize=14, fontweight='bold')
plt.xlabel('Model', fontsize=12)
plt.ylabel('Accuracy', fontsize=12)
plt.ylim([0, 1])

for bar, acc in zip(bars, accuracies):
    height = bar.get_height()
    plt.text(bar.get_x() + bar.get_width()/2., height + 0.02,
             f'{acc*100:.2f}%', ha='center', va='bottom', fontweight='bold', fontsize=11)

plt.grid(True, alpha=0.3, axis='y')
plt.tight_layout()
plt.savefig('model_comparison.png', dpi=300, bbox_inches='tight')
plt.close()

print('\n=== Final Comparison ===')
for name, acc in results.items():
    print(f'{name}: {acc*100:.2f}%')

