from flask import Flask, request, jsonify
import joblib
import numpy as np

# Initialize the Flask application
app = Flask(__name__)

# Load your pre-trained KNN model
# Make sure 'knn_model.pkl' is in the same directory as this script
try:
    model = joblib.load('knn_m.pkl')
    # Example class names, adjust as needed for your model
    class_names = ['Safe', 'Moderate', 'High Risk'] 
    print("✅ Model loaded successfully!")
except FileNotFoundError:
    model = None
    print("❌ Error: 'knn.pkl' not found. Predictions will be simulated.")

@app.route('/predict', methods=['GET'])
def predict():
    # Get sensor data from the request's query parameters
    mq135 = request.args.get('mq135', type=float)
    mq136 = request.args.get('mq136', type=float)
    mq137 = request.args.get('mq137', type=float)

    if mq135 is None or mq136 is None or mq137 is None:
        return "Error: Missing sensor data. Please provide 'mq135' and 'mq7'.", 400

    if model:
        # Prepare the data for the model (needs to be in a 2D array)
        features = np.array([[mq135, mq136, mq137]])
        
        # Get the prediction index from the model
        prediction_index = model.predict(features)[0]
        
        # Map the index to the class name
        prediction_text = class_names[prediction_index]
        print(f"Received: [MQ135: {mq135}, MQ136: {mq136},  MQ137: {mq137}] -> Predicted: {prediction_text}")
        
    else:
        # Fallback for when the model file is not found
        prediction_text = "Simulated Prediction"
        print("Model not loaded. Returning a simulated prediction.")

    # Return the prediction as a simple string
    return prediction_text

if __name__ == '__main__':
    # Run the server, accessible on your local network
    # 0.0.0.0 makes it accessible from other devices on the same network
    app.run(host='0.0.0.0', port=5000, debug=True)