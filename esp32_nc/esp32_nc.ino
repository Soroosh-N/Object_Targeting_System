#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi credentials (laptop hotspot)
const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWORD";

// MQTT Broker IP (your laptop's IP address)
const char* mqtt_server = "192.168.137.1";

// MQTT topic to subscribe to
const char* topic = "esp32cam/latest_data";

// MQTT & WiFi clients
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  Serial.println("🔌 Connecting to WiFi...");
  WiFi.begin(ssid, password);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected");
    Serial.print("📶 IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Failed to connect to WiFi");
  }
}

// MQTT message callback
void callback(char* topic, byte* payload, unsigned int length) {
  // Serial.print("📨 MQTT Received [");
  // Serial.print(topic);
  // Serial.print("]: ");

  // Parse JSON payload
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.println("⚠️ JSON parsing failed");
    return;
  }

  int x = doc["x"];
  int y = doc["y"];
  // const char* time = doc["time"];

  Serial.printf("x%d\r\ny%d\r\n", x, y);

  // Send to UART2 in formatted form
  Serial2.printf("x%d\r\ny%d\r\n", x, y);
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("🔁 Connecting to MQTT broker...");
    if (client.connect("ESP32_UART_Client")) {
      Serial.println("✅ MQTT connected");
      client.subscribe(topic);
    } else {
      Serial.print("❌ failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 2s...");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);    // USB serial monitor
  Serial2.begin(115200);     // UART to external device (TX2=GPIO17, RX2=GPIO16)

  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
}
