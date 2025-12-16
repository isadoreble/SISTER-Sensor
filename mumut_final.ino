#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// === Konfigurasi WiFi ===
#define WIFI_SSID     "HAFIZ AL GIBRAN"
#define WIFI_PASSWORD "Gibran19"

// === Telegram Bot ===
#define BOT_TOKEN "8543449627:AAE2DDk2srQ-OcbU_8JabtjoUhw3QLshBzA"
#define CHAT_ID "1150170072"

// === Link Firebase ===
const char* firebaseHost = "https://sisdig-apartment-default-rtdb.firebaseio.com";

// === Pin Sensor dan Aktuator ===
#define LDR_PIN       33
#define GAS_PIN       34
#define LED_PIN       12
#define BUZZER_PIN    14
#define DHT_PIN       4

#define FAN_IN1_PIN   25
#define FAN_IN2_PIN   26

// === Inisialisasi Sensor DHT22 ===
#define DHTTYPE DHT22
DHT dht(DHT_PIN, DHTTYPE);

// === Variabel anti spam ===
bool gasAlertSent = false;
bool tempAlertSent = false;

// Timer untuk reset auto alert GAS
unsigned long lastGasReset = 0;

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(FAN_IN1_PIN, OUTPUT);
  pinMode(FAN_IN2_PIN, OUTPUT);

  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi terhubung");
}

// === Fungsi kirim Telegram ===
void sendTelegram(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Encode agar karakter aman
    message.replace(" ", "%20");
    message.replace("\n", "%0A");
    message.replace("🚨", "%F0%9F%9A%A8");
    message.replace("🧯", "%F0%9F%A7%AF");
    message.replace("🔥", "%F0%9F%94%A5");

    String url = "https://api.telegram.org/bot";
    url += BOT_TOKEN;
    url += "/sendMessage?chat_id=";
    url += CHAT_ID;
    url += "&text=" + message;

    Serial.println("URL TELEGRAM:");
    Serial.println(url);

    http.begin(url);
    int httpResponse = http.GET();

    Serial.print("Telegram Response: ");
    Serial.println(httpResponse);

    http.end();
  }
}

void loop() {
  // === Baca Sensor ===
  int ldrValue = analogRead(LDR_PIN);
  int gasValue = analogRead(GAS_PIN);
  float suhu = dht.readTemperature();

  // === Serial Monitor ===
  Serial.print("LDR: "); Serial.print(ldrValue);
  Serial.print(" | Suhu: "); Serial.print(suhu);
  Serial.print(" C | GAS: "); Serial.println(gasValue);

  // === LDR → LED ===
  digitalWrite(LED_PIN, (ldrValue > 3000) ? HIGH : LOW);

  // === AUTO RESET ALERT GAS SETIAP 10 DETIK ===
  if (millis() - lastGasReset > 10000) {
    gasAlertSent = false;
    lastGasReset = millis();
  }

  // === GAS ALERT ===
  if (gasValue > 600) {
    digitalWrite(BUZZER_PIN, HIGH);

    Serial.println("MASUK ALERT GAS!!");  // DEBUG PENTING

    if (!gasAlertSent) {
      Serial.println("KIRIM TELEGRAM GAS"); // DEBUG PENTING

      sendTelegram(
        "🚨 PERINGATAN GAS!\n"
        "🧯 Nilai Gas: " + String(gasValue)
      );
      gasAlertSent = true;
    }

  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // === SUHU PANAS ALERT ===
  if (suhu > 30) {
    digitalWrite(FAN_IN1_PIN, HIGH);
    digitalWrite(FAN_IN2_PIN, LOW);

    if (!tempAlertSent) {
      sendTelegram(
        "🔥 SUHU TINGGI!\n"
        "🌡 Suhu: " + String(suhu) + " °C"
      );
      tempAlertSent = true;
    }
  } else {
    digitalWrite(FAN_IN1_PIN, LOW);
    digitalWrite(FAN_IN2_PIN, LOW);
    tempAlertSent = false;
  }

  // === Kirim ke Firebase ===
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = String(firebaseHost) + "/sensor.json";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String dataJson = "{";
    dataJson += "\"ldr\":" + String(ldrValue) + ",";
    dataJson += "\"gas\":" + String(gasValue);

    if (!isnan(suhu)) {
      dataJson += ", \"suhu\":" + String(suhu);
    }

    dataJson += "}";

    int httpResponseCode = http.PUT(dataJson);

    Serial.print("Firebase response code: ");
    Serial.println(httpResponseCode);

    http.end();
  } else {
    Serial.println("WiFi tidak terhubung. Data tidak dikirim.");
  }

  delay(2000);
}