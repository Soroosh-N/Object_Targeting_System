# Remote Object Targeting System

![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Platform](https://img.shields.io/badge/platform-STM32%20%7C%20ESP32-lightgrey.svg)
![Language](https://img.shields.io/badge/language-C++%20%7C%20Python-orange.svg)

---

## Overview 📖

This project implements a real-time system for detecting an object using a camera (ESP32-CAM) and pointing a laser at the detected object through a servo-controlled platform (STM32). The system uses wireless communication, computer vision, and embedded control to achieve its goals.

Key technologies:
- Embedded systems (ESP32, STM32)
- Wireless communication (Wi-Fi, MQTT)
- Computer Vision (OpenCV)
- Web technologies (Flask, XAMPP, MySQL)

The project is the result of a mini-project proposed in Spring 2025 at the University of Utah.

*(View the final report of the project here: [Project Report](./Final%20Report.pdf))*

---

## Project Architecture 🧩

```
ESP32-CAM  (image capture)
   ↓ (socket connection)
Laptop (Flask server)
   ↓ (MQTT publish: object center coordinates)
ESP32  (MQTT subscribe + UART transmission)
   ↓ (UART)
STM32  (servo control to point laser)
```

A local **web dashboard** in the laptop visualizes the object detection results.

<p align="center">
  <img src="https://github.com/Soroosh-N/Object_Targeting_System/blob/main/res/block_diagram.PNG?raw=true" width="70%" alt="Project Architecture"/>
</p>

---

## Repository Structure 📁

```
./
├── mosquitto.conf         # Configuration file for Mosquitto MQTT broker
├── README.md               # Project documentation
├── Report.pdf              # Project report
├── esp32_cam/              # ESP32-CAM firmware (image streaming)
│   └── esp32_cam.ino
├── esp32_nc/               # ESP32 Node Client firmware (receives coordinates, sends to STM32)
│   └── esp32_nc.ino
├── res/                    # Resources
│   ├── Project-Handbook.pdf
│   ├── video_report.m4v
│   └── datasheet ESP32/ | datasheet ESP32_CAM/ | datasheet STM32/
├── server/                 # Laptop server
│   ├── server.py           # Flask server for image reception and processing (object detection)
│   ├── static/
│   └── templates/
│       └── index.html      # Frontend page for displaying results
├── stm32/                  # STM32 firmware project (PlatformIO based)
    ├── Drivers/
    ├── Inc/                # Included libraries for STM32
    └── Src/                # Main folder for source codes
        └── main.c          # Main code for STM32
    └── OtherFolders        # STM32 firmware codes
```

---

## Setup Instructions 🛠️

### Prerequisites 📋

- **Hardware**
  - ESP32-CAM
  - ESP32 DevKit
  - STM32F0 development board
  - Laptop (for server and broker)

- **Software**
  - Python 3.x
  - Arduino IDE (for ESP32 and ESP32-CAM programming)
  - PlatformIO (for STM32 programming)
  - Mosquitto MQTT Broker
  - XAMPP (for MySQL and optional web hosting)
  - Required Python packages:
    ```bash
    pip install flask flask-socketio opencv-python mysql-connector-python paho-mqtt
    ```

---

## Running the System ⚙️

### 1. STM32 Setup

- Open the `stm32/` project with PlatformIO.
- Flash the firmware to the STM32 device.
- Connect UART lines between ESP32 and STM32 properly.

### 2. ESP32 Node Client Setup

- Upload `esp32_nc/esp32_nc.ino` to the ESP32 (Set SSID and Password of your WIFI connection first).
- ESP32 subscribes to MQTT topic `esp32cam/latest_data` and sends data over UART to STM32.

### 3. ESP32-CAM Setup

- Upload `esp32_cam/esp32_cam.ino` to the ESP32-CAM (Set SSID and Password of your WIFI connection first).
- ESP32-CAM streams JPEG images to the laptop server through socket connection.

⚠️ Ensure both ESP32 and ESP32-CAM connect to the **laptop's hotspot Wi-Fi**.

⚠️ Make sure the server is running before powering the ESP32 devices. Else, reset them after starting the server.

### 4. Laptop Server Setup

- Run Mosquitto MQTT broker with:
    ```bash
    mosquitto -c mosquitto.conf -v
    ```
    Sometimes it might not start because the process is already running. First, try to stop the process, then rerun the above command (Make sure port 1883 is open).

- Create a MySQL database `emb_prj` and table `camera_data`:
  - Start XAMPP, enable MySQL.
  - Create a database `emb_prj`.
  - Create a table `camera_data`:
    ```sql
    CREATE TABLE camera_data (
        id INT AUTO_INCREMENT PRIMARY KEY,
        x INT,
        y INT,
        time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    );
    ```
- Start the Flask server:
    ```bash
    cd server
    python server.py
    ```
- Flask server will receive images at `/upload` endpoint, perform object detection (blue object), update database, publish results to MQTT, and serve a live webpage at:
    ```
    http://192.168.137.1:5000
    ```

### 5. Web Visualization

- `server/templates/index.html` will display:
  - Latest received and processed image
  - Detected object's center on a canvas

---

## System Communication Flow 🔄

| Step | Action |
|---|---|
| 1 | ESP32-CAM streams images → Flask server |
| 2 | Flask server detects object using OpenCV and extracts center (x,y) |
| 3 | Flask server publishes coordinates over MQTT topic `esp32cam/latest_data` |
| 4 | ESP32 subscribes to the topic and sends (x,y) over UART to STM32 |
| 5 | STM32 drives servo motors to point a laser at the detected object |

---

## How to Run the Full System 🏃‍♂️

1. Power up STM32, ESP32, and ESP32-CAM.
2. Connect laptop, ESP32, and ESP32-CAM to the same Wi-Fi hotspot.
3. Start Mosquitto MQTT broker on laptop.
4. Start MySQL server via XAMPP.
5. Launch Flask server (`python server.py`).
6. Open the web dashboard at `http://192.168.137.1:5000`.
7. Verify that the ball is visible and tracking.
8. Watch the laser pointer move accordingly!

---

## Calibration Notes 🎯

- The camera and laser pointer are positioned differently.
- Manual calibration between camera coordinates and laser pointer coordinates is necessary.
- Planar homography was considered but not implemented due to project time constraints.

---

## Future Improvements 🚀

- Implement homography-based auto-calibration
- Improve color detection robustness
- Complete laser hit detection by recognizing a red dot via OpenCV

---

## Project Demo Video 🎥

[Watch the Project Demo](res/video_report.m4v)

*(You can click the link to download and view the demonstration video.)*

---

## Team Members 👨‍💻👨‍💻👨‍💻

- Jon Dromey - B.S. at the Mechanical Engineering Department
- Zixuan Wang - M.S. at the Electrical and Computer Engineering Department
- Soroosh Noorzad - Ph.D. at the Electrical and Computer Engineering Department

**Supervisor**: Pierre-Emmanuel Gaillardon

**University of Utah - Spring 2025**

---

## Group Photo 📸

<p align="center">
  <img src="https://github.com/Soroosh-N/Object_Targeting_System/blob/main/res/group_photo.webp?raw=true" width="70%" alt="Team Photo"/>
</p>