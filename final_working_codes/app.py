from flask import Flask, request, jsonify
import joblib
import numpy as np

# Initialize the Flask application
app = Flask(__name__)

# We need the model, the scaler, and the label encoder
try:
    model = joblib.load('knn.pkl')
    scaler = joblib.load('scaler.pkl')
    label_encoder = joblib.load('label_encoder.pkl')
    print("✅ Model, Scaler, and Label Encoder loaded successfully!")
except FileNotFoundError:
    model = None
    scaler = None
    label_encoder = None
    print("❌ Error: 'knn.pkl', 'scaler.pkl', or 'label_encoder.pkl' not found.")
    print("Predictions will be simulated.")


@app.route('/predict', methods=['GET'])
def predict():
    # Get sensor data from the request's query parameters
    mq3 = request.args.get('mq3', type=float)
    mq136 = request.args.get('mq136', type=float)
    mq137 = request.args.get('mq137', type=float)

    if mq3 is None or mq136 is None or mq137 is None:
        return "Error: Missing sensor data. Please provide 'mq3', 'mq136', 'mq137'.", 400

    if model and scaler and label_encoder:
        # Prepare the data for the model (needs to be in a 2D array)
        features = np.array([[mq3, mq136, mq137]])
        
        # The model was trained on scaled data, so new data must be scaled
        features_scaled = scaler.transform(features)
        
        # Get the prediction index (e.g., 0, 1, 2) from the model
        prediction_encoded = model.predict(features_scaled)[0]
        
        # Decode the prediction into the actual gas name
        prediction_gas_name = label_encoder.inverse_transform([prediction_encoded])[0]
        
        print(f"Received: [MQ3: {mq3}, MQ136: {mq136}, MQ137: {mq137}] -> Scaled: {features_scaled} -> Encoded: {prediction_encoded} -> Predicted: {prediction_gas_name}")
        
        # Return the gas name (which is a string)
        return prediction_gas_name
        
    else:
        # Fallback for when the model file is not found
        prediction_gas_name = "Simulated"
        print("Model/Scaler/Encoder not loaded. Returning a simulated prediction.")
        
        return prediction_gas_name

if __name__ == '__main__':
    # Run the server, accessible on your local network
    app.run(host='0.0.0.0', port=5000, debug=False)