# 🛠️ Smart Air Purifier – How-To-Guide

This guide explains how to set up the Smart Air Purifier system, including hardware wiring, software installation and the project workflow.

---

## 1. 🔌 Hardware Setup

### 🧩 Components Required

| Component         | Quantity | Notes                           |
|------------------|----------|----------------------------------|
| ESP32            | 1        | Wi-Fi enabled microcontroller    |
| BME280 Sensor     | 1        | Measures temperature, humidity, pressure |
| CCS811 Sensor     | 1        | Measures eCO₂ and TVOC           |
| PM Sensor (Dust) | 1        | Analog output connected to GPIO |
| Fan + Motor Driver| 1        | Controlled via DIR and PWM pins |
| 5V Power Supply   | 1        | To power ESP32 and peripherals   |
| 12V Power Supply   | 1        | To power the Fan via Motor Driver  |
| Jumper Wires      | –        | For all connections              |
| Breadboard        | Optional | For prototyping connections      |

### 🔌 ESP32 Pin Connections

| ESP32 Pin | Connected To         |
|-----------|----------------------|
| 36        | PM Sensor Output     |
| 17        | PM Sensor LED Power  |
| 25        | Fan Direction (DIR1) |
| 26        | Fan Speed (PWM1)     |
| 21 (SDA)  | I2C SDA (BME280/CCS811) |
| 22 (SCL)  | I2C SCL (BME280/CCS811) |

---

## 2. 💻 Software Installation

### 🧰 Prerequisites

- [Arduino IDE](https://www.arduino.cc/en/software)
- USB cable for ESP32

### 📦 Step-by-Step Instructions

#### ✅ Add ESP32 Support

1. Open Arduino IDE.
2. Go to **File > Preferences**.
3. In the *Additional Board URLs*, add: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
4. Go to **Tools > Board > Board Manager**.
5. Search for **ESP32** and install **"ESP32 by Espressif Systems"**.

#### ✅ Install Required Libraries

In **Sketch > Include Library > Manage Libraries**, search and install:

- `Adafruit BME280`
- `Adafruit Unified Sensor`
- `CCS811 by SparkFun`
- `WiFi`
- `WebServer`
- `HTTPClient`
- `Preferences`

#### ✅ Upload Code

1. Connect your ESP32 via USB.
2. Select board: `ESP32 Dev Module`
3. Select port: **Tools > Port**
4. Open `air_purifier.ino` and click **Upload**.

---

## 3. 🌐 Project Working

### 📡 Wi-Fi Setup

- On first boot or when Wi-Fi credentials are missing:
- ESP32 creates a hotspot:
 - **SSID**: `ESP32_Setup`
 - **Password**: `12345678`
- Connect from your phone/PC and enter Wi-Fi credentials via a web page.

### 🌫️ Sensor Monitoring

ESP32 collects the following environmental data:
- **Temperature**, **Humidity**, **Pressure** from BME280
- **TVOC**, **CO₂** from CCS811
- **Dust level (analog AQI)** via PM sensor

### ☁️ Firebase Integration

All data is sent every second to Firebase Realtime Database: https://air-purifier-56831-default-rtdb.asia-southeast1.firebasedatabase.app/sensor.json


### 💨 Fan Control Logic

The fan is controlled either manually or automatically based on AQI:

- **Manual Mode**: Set speed via Firebase path `/fan/speed`
- **Auto Mode**:
  - **AQI > 200** → High Speed
  - **AQI 150–200** → Medium Speed
  - **AQI < 150** → Off

Thresholds and modes are configured via:  
/fan/mode  
/fan/threshold  
/fan/tvocThreshold  


### 📊 Web Dashboard

A responsive web interface (`index.html`) displays:

- Real-time sensor data
- Fan control (auto/manual)
- Historical charts (AQI, TVOC, Temp, CO₂)
- Login & Google OAuth support via Firebase

> This frontend is hosted on **GitHub Pages** and pulls data directly from Firebase.

---

## ✅ Live Demo

Access the deployed web interface via: https://manmikalpha.github.io/

## 📘 Frontend Setup

To set up and run the frontend (web interface with dashboard and login), please follow the `README.md` provided in the GitHub repository (https://github.com/manmikalpha/manmikalpha.github.io/blob/main/README.md)

It includes instructions for:
- Firebase Authentication (including Google OAuth)
- Using Live Server for local testing
- Connecting the dashboard to the ESP32 via Firebase

Make sure you replace Firebase config values and deploy credentials accordingly.

---

## 📦 Notes

- Make sure to secure your Firebase rules before deploying publicly.
- Adjust sampling intervals and thresholds in code for production use.

---

## 📄 License

MIT License. Free to use and modify.



