/*
  AlertaGYE - Water-level sensor node (Wokwi simulation)

  An ultrasonic sensor mounted above a drain, a channel or the street
  measures the distance to the water surface. When the water rises, the
  distance drops. The node turns that into a water level and publishes it
  over MQTT.

  In Wokwi: click the sensor while the simulation runs and move the
  distance slider to make the water rise or fall.
*/

#include <WiFi.h>
#include <PubSubClient.h>

// ---------- Settings ----------
const char* WIFI_SSID = "Wokwi-GUEST";   // Wokwi's simulated network
const char* WIFI_PASS = "";

const char* MQTT_BROKER = "broker.hivemq.com";
const int   MQTT_PORT   = 1883;

// Node identity: must match the point ids in the GeoJSON
const char* SENSOR_ID = "florida_norte";
const char* ZONE      = "via_daule";

// Publishing topic: alertagye/sensors/<SENSOR_ID>
String topic = String("alertagye/sensors/") + SENSOR_ID;

// Installation geometry (cm)
const float SENSOR_HEIGHT_CM  = 200.0;  // distance from the sensor to the dry ground
const float CRITICAL_LEVEL_CM = 30.0;   // water level considered a flood

// Pins
const int PIN_TRIG     = 5;
const int PIN_ECHO     = 18;
const int LED_GREEN    = 25;
const int LED_YELLOW   = 26;
const int LED_RED      = 27;

const unsigned long INTERVAL_MS = 3000;  // in the field: every 5 min when dry, every 1 min when raining

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
unsigned long lastSend = 0;

// ---------- Measurement ----------
float measureDistanceCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duration = pulseIn(PIN_ECHO, HIGH, 30000);  // 30 ms timeout (~5 m)
  if (duration == 0) return -1;                    // no echo: invalid reading
  return duration * 0.0343 / 2.0;
}

// Average of 5 readings to filter noise (waves, rain, floating debris)
float measureFilteredDistance() {
  float sum = 0;
  int valid = 0;
  for (int i = 0; i < 5; i++) {
    float d = measureDistanceCm();
    if (d > 0) { sum += d; valid++; }
    delay(50);
  }
  return valid ? sum / valid : -1;
}

// Same thresholds as the web app: yellow from half the critical level, red at the critical level
const char* computeState(float levelCm) {
  if (levelCm >= CRITICAL_LEVEL_CM) return "red";
  if (levelCm >= CRITICAL_LEVEL_CM / 2) return "yellow";
  return "green";
}

void showLeds(const char* state) {
  digitalWrite(LED_GREEN,  strcmp(state, "green") == 0);
  digitalWrite(LED_YELLOW, strcmp(state, "yellow") == 0);
  digitalWrite(LED_RED,    strcmp(state, "red") == 0);
}

// ---------- Connectivity ----------
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println(" done");
}

void connectMQTT() {
  while (!mqtt.connected()) {
    String clientId = String("alertagye-") + SENSOR_ID + "-" + String(random(0xffff), HEX);
    Serial.print("Connecting to MQTT... ");
    if (mqtt.connect(clientId.c_str())) {
      Serial.println("done");
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqtt.state());
      delay(2000);
    }
  }
}

// ---------- Main program ----------
void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  connectWiFi();
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
}

void loop() {
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  if (millis() - lastSend < INTERVAL_MS) return;
  lastSend = millis();

  float distance = measureFilteredDistance();
  if (distance < 0) {
    Serial.println("Invalid reading, skipped");
    return;
  }

  float level = SENSOR_HEIGHT_CM - distance;
  if (level < 0) level = 0;
  float pct = level / CRITICAL_LEVEL_CM * 100.0;
  if (pct > 100) pct = 100;

  const char* state = computeState(level);
  showLeds(state);

  char payload[200];
  snprintf(payload, sizeof(payload),
    "{\"sensor_id\":\"%s\",\"zone\":\"%s\",\"distance_cm\":%.1f,"
    "\"level_cm\":%.1f,\"pct\":%.0f,\"state\":\"%s\"}",
    SENSOR_ID, ZONE, distance, level, pct, state);

  mqtt.publish(topic.c_str(), payload);
  Serial.print(topic);
  Serial.print(" -> ");
  Serial.println(payload);
}
