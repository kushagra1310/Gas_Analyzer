import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import LabelEncoder, StandardScaler
from sklearn.neighbors import KNeighborsClassifier
from sklearn.ensemble import RandomForestClassifier, GradientBoostingClassifier
from sklearn.svm import SVC
from sklearn.tree import DecisionTreeClassifier
from sklearn.naive_bayes import GaussianNB
from sklearn.metrics import accuracy_score
import matplotlib.pyplot as plt
import seaborn as sns
from tensorflow import keras
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import LSTM, Dense, Dropout
from tensorflow.keras.utils import to_categorical

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

# Standardize features
scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

# --- LSTM Specific Preprocessing ---
y_categorical = to_categorical(y_encoded)
X_train_lstm, X_test_lstm, y_train_lstm, y_test_lstm = train_test_split(X, y_categorical, test_size=0.2, random_state=42, stratify=y_encoded)
X_train_scaled_lstm = scaler.fit_transform(X_train_lstm)
X_test_scaled_lstm = scaler.transform(X_test_lstm)
X_train_lstm_reshaped = X_train_scaled_lstm.reshape(X_train_scaled_lstm.shape[0], 1, X_train_scaled_lstm.shape[1])
X_test_lstm_reshaped = X_test_scaled_lstm.reshape(X_test_scaled_lstm.shape[0], 1, X_test_scaled_lstm.shape[1])


# Initialize models with best parameters from your scripts
models = {
    'KNN': KNeighborsClassifier(n_neighbors=5, weights='distance', metric='manhattan'),
    'Random Forest': RandomForestClassifier(n_estimators=200, max_depth=20, min_samples_leaf=1, min_samples_split=2, max_features='sqrt', random_state=42),
    'SVM': SVC(kernel='rbf', C=10, gamma='scale', random_state=42),
    'Decision Tree': DecisionTreeClassifier(criterion='entropy', max_depth=15, min_samples_leaf=1, min_samples_split=2, random_state=42),
    'Gradient Boosting': GradientBoostingClassifier(n_estimators=150, learning_rate=0.2, max_depth=7, subsample=1.0, random_state=42),
    'Gaussian NB': GaussianNB(var_smoothing=1.0)
}

# --- Build and Compile LSTM Model ---
lstm_model = Sequential([
    LSTM(128, input_shape=(1, 7), return_sequences=True),
    Dropout(0.3),
    LSTM(64, return_sequences=False),
    Dropout(0.3),
    Dense(64, activation='relu'),
    Dropout(0.2),
    Dense(32, activation='relu'),
    Dense(4, activation='softmax')
])
lstm_model.compile(optimizer='adam', loss='categorical_crossentropy', metrics=['accuracy'])

# Train and evaluate
results = {}
for name, model in models.items():
    print(f'\nTraining {name}...')
    model.fit(X_train_scaled, y_train)
    y_pred = model.predict(X_test_scaled)
    accuracy = accuracy_score(y_test, y_pred)
    results[name] = accuracy
    print(f'{name} Accuracy: {accuracy*100:.2f}%')

# --- Train and Evaluate LSTM ---
print('\nTraining LSTM...')
history = lstm_model.fit(X_train_lstm_reshaped, y_train_lstm,
                         epochs=50,
                         batch_size=32,
                         validation_split=0.2,
                         verbose=0) # Set verbose to 0 to reduce output
test_loss, test_accuracy = lstm_model.evaluate(X_test_lstm_reshaped, y_test_lstm, verbose=0)
results['LSTM'] = test_accuracy
print(f'LSTM Accuracy: {test_accuracy*100:.2f}%')


# Plot comparison
plt.figure(figsize=(12, 7))
models_list = list(results.keys())
accuracies = list(results.values())
colors = ['#3498DB', '#2ECC71', '#E74C3C', '#F1C40F', '#9B59B6', '#34495E', '#1ABC9C']

bars = plt.bar(models_list, accuracies, color=colors, alpha=0.8)
plt.title('Model Comparison: Gas Detection Accuracy', fontsize=16, fontweight='bold')
plt.xlabel('Model', fontsize=12)
plt.ylabel('Accuracy', fontsize=12)
plt.ylim([0, 1.1])

for bar, acc in zip(bars, accuracies):
    height = bar.get_height()
    plt.text(bar.get_x() + bar.get_width()/2., height + 0.02,
             f'{acc*100:.2f}%', ha='center', va='bottom', fontweight='bold', fontsize=11)

plt.grid(True, alpha=0.3, axis='y')
plt.xticks(rotation=45)
plt.tight_layout()
plt.savefig('all_model_comparison.png', dpi=300, bbox_inches='tight')
plt.close()

print('\n=== Final Comparison ===')
for name, acc in results.items():
    print(f'{name}: {acc*100:.2f}%')