#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <CCS811.h>
#include <time.h>

Preferences preferences;
WebServer server(80);
Adafruit_BME280 bme;
CCS811 sensor;

String ssid = "", password = "";

const char* ap_ssid = "ESP32_Setup";
const char* ap_password = "12345678";

const char* FIREBASE_HOST = "https://air-purifier-56831-default-rtdb.asia-southeast1.firebasedatabase.app/sensor.json";
const char* FAN_CONTROL_PATH = "https://air-purifier-56831-default-rtdb.asia-southeast1.firebasedatabase.app/fan/speed.json";

const int measurePin = 36;
const int ledPower = 17;
unsigned int samplingTime = 280;
unsigned int deltaTime = 40;
unsigned int sleepTime = 9680;

const int DIR1 = 25;
const int PWM1 = 26;

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 1000;

// AQI thresholds with hysteresis
const int AQI_HIGH_ON = 200;
const int AQI_HIGH_OFF = 180;

const int AQI_MEDIUM_ON = 101;
const int AQI_MEDIUM_OFF = 90;

enum FanLevel { FAN_OFF, FAN_LOW, FAN_MEDIUM, FAN_HIGH };
FanLevel currentFanLevel = FAN_LOW;  // default starting point


// HTML content for the configuration page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Wi-Fi Setup</title>
  <style>
    body { font-family: Arial, sans-serif; background-color: #f2f2f2; padding: 20px; }
    h2 { text-align: center; }
    form {
      max-width: 300px;
      margin: auto;
      padding: 20px;
      background-color: #fff;
      box-shadow: 0px 0px 10px rgba(0,0,0,0.1);
      border-radius: 8px;
    }
    input[type=text], input[type=password] {
      width: 100%;
      padding: 12px 20px;
      margin: 8px 0;
      display: inline-block;
      border: 1px solid #ccc;
      border-radius: 4px;
      box-sizing: border-box;
    }
    input[type=submit] {
      width: 100%;
      background-color: #4CAF50;
      color: white;
      padding: 14px 20px;
      margin-top: 10px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
    }
    input[type=submit]:hover {
      background-color: #45a049;
    }
  </style>
</head>
<body>
  <h2>ESP32 Wi-Fi Configuration</h2>
  <form action="/save" method="post">
    <label for="ssid">SSID:</label>
    <input type="text" id="ssid" name="ssid" required>
    <label for="password">Password:</label>
    <input type="password" id="password" name="password" required>
    <input type="submit" value="Save & Connect">
  </form>
</body>
</html>
)rawliteral";

// Function to calculate AQI based on PM2.5 concentration
int calculateAQI_PM25(float pm25) {
  struct AQIBreakpoint {
    float cLow, cHigh;
    int aLow, aHigh;
  };

  // Breakpoints for AQI calculation
  const AQIBreakpoint breakpoints[] = {
    {0.0, 12.0, 0, 50},
    {12.1, 35.4, 51, 100},
    {35.5, 55.4, 101, 150},
    {55.5, 150.4, 151, 200},
    {150.5, 250.4, 201, 300},
    {250.5, 500.4, 301, 500}
  };

  for (auto bp : breakpoints) {
    if (pm25 >= bp.cLow && pm25 <= bp.cHigh) {
      return round(((bp.aHigh - bp.aLow) / (bp.cHigh - bp.cLow)) * (pm25 - bp.cLow) + bp.aLow);
    }
  }
  return 500;
}

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleSave() {
  ssid = server.arg("ssid");
  password = server.arg("password");

  if (ssid.length() > 0 && password.length() > 0) {
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    server.send(200, "text/html", "<h2>Saved. Rebooting...</h2>");
    delay(2000);
    ESP.restart();
  } else {
    server.send(400, "text/html", "<h2>Invalid input. Try again.</h2>");
  }
}

// Start the Access Point mode
void startAPMode() {
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reset", HTTP_POST, handleReset);
  server.onNotFound([]() {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
  });

  server.begin();
  Serial.println("HTTP server started");
}

// Connect to Wi-Fi using stored credentials
void connectToWiFi() {
  preferences.begin("wifi", true);
  String stored_ssid = preferences.getString("ssid", "");
  String stored_password = preferences.getString("password", "");
  preferences.end();

  if (stored_ssid != "" && stored_password != "") {
    WiFi.begin(stored_ssid.c_str(), stored_password.c_str());
    Serial.print("Connecting to WiFi");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      Serial.print(".");
      delay(500);
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to WiFi!");
      Serial.println(WiFi.localIP());

      server.on("/", handleRoot);
      server.on("/save", HTTP_POST, handleSave);
      server.on("/reset", HTTP_POST, handleReset);
      server.onNotFound([]() {
        server.sendHeader("Location", String("http://") + WiFi.localIP().toString(), true);
        server.send(302, "text/plain", "");
      });

      server.begin();
      Serial.println("HTTP server started in STA mode");
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      return;
    } else {
      Serial.println("\nFailed to connect. Starting AP mode.");
    }
  }
  startAPMode();
}


void handleReset() {
  Serial.println("Reset triggered from client.");
  WiFi.disconnect(true);
  preferences.begin("wifi", false);
  preferences.clear();
  preferences.end();
  server.send(200, "text/html", "<h2>Wi-Fi credentials cleared. Rebooting...</h2>");
  delay(2000);
  ESP.restart();
}

// Function to send sensor data to Firebase
void sendToFirebase() {
  if (WiFi.status() == WL_CONNECTED) {
    pinMode(ledPower, OUTPUT);
    digitalWrite(ledPower, LOW);
    delayMicroseconds(samplingTime);
    float voMeasured = analogRead(measurePin);
    delayMicroseconds(deltaTime);
    digitalWrite(ledPower, HIGH);
    delayMicroseconds(sleepTime);

    float calcVoltage = voMeasured * (3.3 / 4095.0);
    float dustDensity = (calcVoltage - 0.2) * 161.29;
    if (dustDensity < 0) dustDensity = 0.0;

    int aqi = calculateAQI_PM25(dustDensity);

    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F;

    int co2 = 0, tvoc = 0;
    if (sensor.checkDataReady()) {
      co2 = sensor.getCO2PPM();
      tvoc = sensor.getTVOCPPB();
    }

    time_t now = time(nullptr);

    String json = "{";
    json += "\"temperature\": " + String(temperature, 2) + ", ";
    json += "\"humidity\": " + String(humidity, 2) + ", ";
    json += "\"pressure\": " + String(pressure, 2) + ", ";
    json += "\"co2\": " + String(co2) + ", ";
    json += "\"tvoc\": " + String(tvoc) + ", ";
    json += "\"dust\": " + String(dustDensity, 2) + ", ";
    json += "\"aqi\": " + String(aqi) + ", ";
    json += "\"lastSeen\": " + String((long)now);
    json += "}";
    HTTPClient firebaseHttp;
    firebaseHttp.begin(FIREBASE_HOST);
    firebaseHttp.addHeader("Content-Type", "application/json");
    int httpCode = firebaseHttp.PUT(json);
    if (httpCode > 0) {
      Serial.printf("Firebase PUT success, code: %d\n", httpCode);
      Serial.println(firebaseHttp.getString());
    } else {
      Serial.printf("Firebase PUT failed: %s\n", firebaseHttp.errorToString(httpCode).c_str());
    }
    firebaseHttp.end();

    static unsigned long lastHistorySave = 0;
    if (millis() - lastHistorySave >= 30000) {
      lastHistorySave = millis();

      time_t now = time(nullptr);
      String path = "/sensor_history/" + String(now);  // Key is UNIX timestamp
      String json = "{";
      json += "\"aqi\": " + String(aqi) + ", ";
      json += "\"tvoc\": " + String(tvoc) + ", ";
      json += "\"temperature\": " + String(temperature, 2) + ", ";
      json += "\"co2\": " + String(co2);
      json += "}";


      HTTPClient firebaseHttp;
      firebaseHttp.begin("https://air-purifier-56831-default-rtdb.asia-southeast1.firebasedatabase.app" + path + ".json");
      firebaseHttp.addHeader("Content-Type", "application/json");
      int code = firebaseHttp.PUT(json);
      if (code > 0) {
        Serial.printf("Logged history (%ld): AQI=%d, TVOC=%d\n", now, aqi, tvoc);
      } else {
        Serial.printf("Failed to log history: %s\n", firebaseHttp.errorToString(code).c_str());
      }
      firebaseHttp.end();
    }

  }
}

// Function to fetch fan speed from Firebase
void fetchFanSpeedFromFirebase() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient firebaseHttp;
    firebaseHttp.begin(FAN_CONTROL_PATH);
    int httpCode = firebaseHttp.GET();

    if (httpCode == 200) {
      String payload = firebaseHttp.getString();
      int fanLevel = payload.toInt();
      Serial.printf("Received fan speed: %d\n", fanLevel);

      int fanPWM;
      switch (fanLevel) {
        case 1: fanPWM = 232; break;
        case 2: fanPWM = 237; break;
        case 3: fanPWM = 255; break;
        default: fanPWM = 0; break;
      }

      analogWrite(PWM1, fanPWM);
    } else {
      Serial.printf("Failed to fetch fan speed: %s\n", firebaseHttp.errorToString(httpCode).c_str());
    }

    firebaseHttp.end();
  }
}

// Function to determine fan level based on TVOC value
FanLevel getTvocFanLevel(int tvoc) {
  if (tvoc < 220) return FAN_LOW;
  else if (tvoc <= 1430) return FAN_MEDIUM;
  else return FAN_HIGH;
}


void setup() {
  Serial.begin(115200);
  delay(1000);
  // Initialize Preferences
  pinMode(ledPower, OUTPUT);
  pinMode(DIR1, OUTPUT);
  pinMode(PWM1, OUTPUT);
  digitalWrite(DIR1, LOW);
  analogWrite(PWM1, 0);
  bme.begin(0x76);
  while (sensor.begin() != 0) {
    Serial.println("CCS811 init failed. Check wiring.");
    delay(1000);
  }
  sensor.setMeasCycle(sensor.eCycle_250ms);
  connectToWiFi();
  if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) {
  Serial.print("Waiting for NTP time sync...");
  time_t now = time(nullptr);
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  while (now < 100000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\nTime synced.");
} else {
  Serial.println("Skipping NTP time sync: AP mode or no WiFi.");
}

}

void loop() {
  HTTPClient firebaseHttp;
  server.handleClient();
  if (millis() - lastSendTime > sendInterval) {
    lastSendTime = millis();
    sendToFirebase();

    // Fetch fan mode and threshold
    if (WiFi.status() == WL_CONNECTED) {
      firebaseHttp.begin("https://air-purifier-56831-default-rtdb.asia-southeast1.firebasedatabase.app/fan/mode.json");
      int modeCode = firebaseHttp.GET();
      String mode = "";
      if (modeCode == 200) {
        mode = firebaseHttp.getString();
        mode.trim();
        mode.replace("\"", ""); // Remove quotes
        Serial.print("Fan mode: ");
        Serial.println(mode);
      }
      firebaseHttp.end();

      // Check if mode is "auto" or "manual"
      if (mode == "auto") {

        firebaseHttp.begin("https://air-purifier-56831-default-rtdb.asia-southeast1.firebasedatabase.app/fan/smart.json");
        int smartCode = firebaseHttp.GET();
        bool smartEnabled = false;
        if (smartCode == 200) {
          String smartStr = firebaseHttp.getString();
          smartEnabled = smartStr == "true";
        }
        firebaseHttp.end();

        // If smart mode is enabled, fetch thresholds and AQI
        firebaseHttp.begin("https://air-purifier-56831-default-rtdb.asia-southeast1.firebasedatabase.app/fan/threshold.json");
        int threshCode = firebaseHttp.GET();
        if (threshCode == 200) {
          String threshStr = firebaseHttp.getString();
          float threshold = threshStr.toFloat();
          Serial.print("Auto threshold: ");
          Serial.println(threshold);
        firebaseHttp.end();

        firebaseHttp.begin("https://air-purifier-56831-default-rtdb.asia-southeast1.firebasedatabase.app/fan/tvocThreshold.json");
        int tvocCode = firebaseHttp.GET();
        float tvocThreshold = 9999; // default high

        if (tvocCode == 200) {
          String tvocStr = firebaseHttp.getString();
          tvocThreshold = tvocStr.toFloat();
          Serial.print("TVOC Threshold: ");
          Serial.println(tvocThreshold);
        }
        firebaseHttp.end();


          int currentTVOC = 0;

        // Read AQI from Firebase
        int aqi = 0;
        firebaseHttp.begin("https://air-purifier-56831-default-rtdb.asia-southeast1.firebasedatabase.app/sensor.json");
        int aqiCode = firebaseHttp.GET();

        if (aqiCode == 200) {
          String payload = firebaseHttp.getString();
          Serial.println("sensor.json payload:");
          Serial.println(payload);

          int start = payload.indexOf("\"aqi\":");
          if (start >= 0) {
            int end = payload.indexOf(",", start);  // works because aqi is followed by comma
            String aqiStr = payload.substring(start + 6, end);
            aqiStr.trim();
            aqi = aqiStr.toInt();
            Serial.print("Parsed AQI: ");
            Serial.println(aqi);
          } else {
            Serial.println("Failed to locate 'aqi' field in payload.");
          }
        } else {
          Serial.print("Failed to GET /sensor.json. Code: ");
          Serial.println(aqiCode);
        }
        firebaseHttp.end();




          // Read TVOC from Firebase
          // For smart mode TVOC reading
          firebaseHttp.begin("https://air-purifier-56831-default-rtdb.asia-southeast1.firebasedatabase.app/sensor/tvoc.json");
          int sensorTvocCode = firebaseHttp.GET();
          currentTVOC = 0;
          if (sensorTvocCode == 200) {
            currentTVOC = firebaseHttp.getString().toInt();
            Serial.print("Firebase TVOC: ");
            Serial.println(currentTVOC);
          }
          firebaseHttp.end();



          if (smartEnabled) {
  // Get fan level for AQI
  FanLevel aqiFanLevel;
  if (aqi >= AQI_HIGH_ON) {
    aqiFanLevel = FAN_HIGH;
  } else if (aqi < AQI_HIGH_OFF && aqi >= AQI_MEDIUM_ON) {
    aqiFanLevel = FAN_MEDIUM;
  } else {
    aqiFanLevel = FAN_LOW;
  }

  // Get fan level for TVOC
  FanLevel tvocFanLevel = getTvocFanLevel(currentTVOC);

  // Choose the higher fan level
  FanLevel newFanLevel = (aqiFanLevel > tvocFanLevel) ? aqiFanLevel : tvocFanLevel;

  if (newFanLevel != currentFanLevel) {
    currentFanLevel = newFanLevel;
    // Change fan speed based on the new level
    switch (currentFanLevel) {
      case FAN_LOW:
        analogWrite(PWM1, 232);
        Serial.println("Smart mode: Fan set to LOW");
        break;
      case FAN_MEDIUM:
        analogWrite(PWM1, 236);
        Serial.println("Smart mode: Fan set to MEDIUM");
        break;
      case FAN_HIGH:
        analogWrite(PWM1, 255);
        Serial.println("Smart mode: Fan set to HIGH");
        break;
      default:
        analogWrite(PWM1, 0);
        Serial.println("Smart mode: Fan turned OFF");
        break;
    }
  } else {
    Serial.println("Smart mode: No fan change (debounced)");
  }
}

           else {
            if (aqi >= threshold || currentTVOC >= tvocThreshold) {
              analogWrite(PWM1, 255);
              Serial.println("Fan ON: Threshold exceeded");
            } else {
              analogWrite(PWM1, 0);
              Serial.println("Fan OFF: Below thresholds");
            }
          }


        }
        
      } else {
        fetchFanSpeedFromFirebase();
      }
    }
  }
}

