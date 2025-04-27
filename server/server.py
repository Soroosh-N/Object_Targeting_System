from flask import Flask, render_template, request
from flask_socketio import SocketIO
import cv2
import numpy as np
import json
import mysql.connector
import paho.mqtt.client as mqtt
from datetime import datetime

# === Setup ===
app = Flask(__name__)
socketio = SocketIO(app)
latest_img_path = "static/processed.jpg"

# === MySQL DB Connection ===
db_conn = mysql.connector.connect(
    host="localhost",
    user="root",
    password="",
    database="emb_prj"
)
db_cursor = db_conn.cursor()

def insert_into_database(x, y):
    query = "INSERT INTO camera_data (x, y) VALUES (%s, %s)"
    db_cursor.execute(query, (x, y))
    db_conn.commit()
    print(f"✅ Saved to DB: x={x}, y={y}")

# === MQTT ===
mqtt_client = mqtt.Client()
mqtt_client.connect("localhost", 1883)
mqtt_topic = "esp32cam/latest_data"

def publish_to_mqtt(x, y):
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    payload = json.dumps({"x": x, "y": y, "time": timestamp})
    mqtt_client.publish(mqtt_topic, payload)
    print(f"📡 Published: {payload}")

# === Routes ===
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/upload', methods=['POST'])
def upload_image():
    img_data = request.data
    img_array = np.frombuffer(img_data, np.uint8)
    img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)

    hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
    # # RED color detection
    # lower_red = np.array([0, 100, 100])
    # upper_red = np.array([10, 255, 255])
    # mask = cv2.inRange(hsv, lower_red, upper_red)
    # 🔵 Blue object detection
    lower_blue = np.array([90, 80, 40])
    upper_blue = np.array([140, 255, 255])
    mask = cv2.inRange(hsv, lower_blue, upper_blue)

    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    center = None

    if contours:
        largest = max(contours, key=cv2.contourArea)
        x, y, w, h = cv2.boundingRect(largest)
        center = (x + w // 2, y + h // 2)
        cv2.rectangle(img, (x, y), (x + w, y + h), (0, 255, 0), 1)  # blue
        cv2.circle(img, center, 1, (0, 0, 255), -1)

        # ✅ DB + MQTT + WebSocket
        insert_into_database(center[0], center[1])
        publish_to_mqtt(center[0], center[1])
        socketio.emit('new_frame', {"x": center[0], "y": center[1]})
    else:
        socketio.emit('new_frame', {})  # no object

    cv2.imwrite(latest_img_path, img)
    return ('', 204)

# === Main Entry Point ===
if __name__ == '__main__':
    try:
        mqtt_client.loop_start()
        socketio.run(app, host='0.0.0.0', port=5000)
    finally:
        db_cursor.close()
        db_conn.close()