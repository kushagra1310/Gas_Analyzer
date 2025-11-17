from flask import Flask, request, jsonify
import joblib
import numpy as np

app = Flask(__name__)

# Load Model Artifacts
try:
    model = joblib.load('knn.pkl')
    scaler = joblib.load('scaler.pkl')
    label_encoder = joblib.load('label_encoder.pkl')
    print("✅ Model, Scaler, and Label Encoder loaded!")
except FileNotFoundError:
    model = None
    print("❌ Error: Model files not found.")

@app.route('/predict', methods=['GET'])
def predict():
    # Get sensor values
    mq3 = request.args.get('mq3', type=float)
    mq136 = request.args.get('mq136', type=float)
    mq137 = request.args.get('mq137', type=float)
    
    # Get slope values
    s_mq3 = request.args.get('s_mq3', type=float)
    s_mq136 = request.args.get('s_mq136', type=float)
    s_mq137 = request.args.get('s_mq137', type=float)

    # Check if all data is present
    if None in [mq3, mq136, mq137, s_mq3, s_mq136, s_mq137]:
        return "Error: Missing parameters. Need mq3, mq136, mq137, s_mq3, s_mq136, s_mq137", 400

    if model:
        # Create feature array with 6 features
        # Ensure your new model was trained in this exact order:
        # [MQ3_Val, MQ136_Val, MQ137_Val, MQ3_Slope, MQ136_Slope, MQ137_Slope]
        features = np.array([[mq3, mq136, mq137, s_mq3, s_mq136, s_mq137]])
        
        # Scale features
        features_scaled = scaler.transform(features)
        
        # Predict
        prediction_idx = model.predict(features_scaled)[0]
        prediction_name = label_encoder.inverse_transform([prediction_idx])[0]
        
        print(f"Input: {features} -> Pred: {prediction_name}")
        return prediction_name
        
    return "Simulated"

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=False)