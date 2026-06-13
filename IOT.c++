#include <Wire.h>
#include <MPU6050.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <HTTPClient.h>

MPU6050 mpu;
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// WiFi
const char* ssid = "KING";
const char* password = "12345678";

// GANTI folder_web sesuai nama folder web Anda di htdocs
String serverName = "http://192.168.137.1/web_project_iot/kirimdata.php";

// Pin
const int buzzerPin = 23;
const int mq5Pin = 34;
const int SDA_PIN = 21;
const int SCL_PIN = 22;

// Threshold
float shockThreshold = 2.0;
int gasThreshold = 800;
int buzzerDuration = 500;

bool gasSensorActive = false;
bool gpsLocked = false;

void kirimData(String event, float lat, float lng, float value) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
  
    String url = serverName +
                 "?event=" + event +
                 "&lat=" + String(lat, 6) +
                 "&lng=" + String(lng, 6) +
                 "&value=" + String(value);

    Serial.println("Kirim ke: " + url);

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.print("HTTP Response: ");
      Serial.println(httpCode);
      Serial.println(http.getString());
    } else {
      Serial.print("Gagal kirim data. Error: ");
      Serial.println(httpCode);
    }

    http.end();
  } else {
    Serial.println("WiFi terputus!");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  pinMode(mq5Pin, INPUT);

  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("Menghubungkan WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Terhubung!");
  Serial.print("IP ESP32: ");
  Serial.println(WiFi.localIP());

  Serial.println("Memulai MPU6050...");
  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("MPU6050 terdeteksi!");
  } else {
    Serial.println("MPU6050 TIDAK terdeteksi!");
    while (1) {
      tone(buzzerPin, 1000);
      delay(200);
      noTone(buzzerPin);
      delay(200);
    }
  }

  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("GPS siap, mencari satelit...");
}

void loop() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (!gpsLocked && gps.location.isValid()) {
    gpsLocked = true;
    gasSensorActive = true;

    Serial.println("GPS LOCK! Sensor gas aktif.");

    for (int i = 0; i < 5; i++) {
      tone(buzzerPin, 2000);
      delay(300);
      noTone(buzzerPin);
      delay(300);
    }
  }

  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float ax_g = ax / 16384.0;
  float ay_g = ay / 16384.0;
  float az_g = az / 16384.0;
  float totalG = sqrt(ax_g * ax_g + ay_g * ay_g + az_g * az_g);

  float lat = gps.location.isValid() ? gps.location.lat() : 0;
  float lng = gps.location.isValid() ? gps.location.lng() : 0;

  if (totalG > shockThreshold) {
    Serial.println("=== Shock Event ===");
    Serial.println(totalG);

    kirimData("shock", lat, lng, totalG);

    tone(buzzerPin, 1500);
    delay(buzzerDuration);
    noTone(buzzerPin);
  }

  if (gasSensorActive) {
    int mq5Value = analogRead(mq5Pin);

    if (mq5Value > gasThreshold) {
      Serial.println("=== Gas Event ===");
      Serial.println(mq5Value);

      kirimData("gas", lat, lng, mq5Value);

      tone(buzzerPin, 2000);
      delay(buzzerDuration);
      noTone(buzzerPin);
    }
  }

  delay(500);
}
