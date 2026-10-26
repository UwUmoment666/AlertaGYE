/*
  AlertaGYE — Nodo sensor de nivel de agua (simulación en Wokwi)

  Un sensor ultrasónico montado sobre una alcantarilla, canal o la calzada
  mide la distancia hasta la superficie del agua. Si el agua sube, la
  distancia baja. Con eso se calcula el nivel y se publica por MQTT.

  En Wokwi: haz clic en el sensor durante la simulación y mueve el
  deslizador de distancia para "subir" o "bajar" el agua.
*/

#include <WiFi.h>
#include <PubSubClient.h>

// ---------- Configuración ----------
const char* WIFI_SSID = "Wokwi-GUEST";   // red simulada de Wokwi
const char* WIFI_PASS = "";

const char* MQTT_BROKER = "broker.hivemq.com";
const int   MQTT_PORT   = 1883;

// Identidad del nodo: debe coincidir con los ids del GeoJSON
const char* SENSOR_ID = "florida_norte";
const char* BARRIO    = "via_daule";

// Tópico donde publica: alertagye/sensores/<SENSOR_ID>
String topico = String("alertagye/sensores/") + SENSOR_ID;

// Geometría de la instalación (en cm)
const float ALTURA_SENSOR   = 200.0;  // distancia del sensor al fondo seco
const float ALTURA_CRITICA  = 30.0;   // nivel de agua que se considera inundación

// Pines
const int PIN_TRIG = 5;
const int PIN_ECHO = 18;
const int LED_VERDE    = 25;
const int LED_AMARILLO = 26;
const int LED_ROJO     = 27;

const unsigned long INTERVALO_MS = 3000;  // en la vida real: 5 min sin lluvia, 1 min con lluvia

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
unsigned long ultimoEnvio = 0;

// ---------- Medición ----------
float medirDistanciaCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duracion = pulseIn(PIN_ECHO, HIGH, 30000);  // timeout 30 ms (~5 m)
  if (duracion == 0) return -1;                    // sin eco: lectura inválida
  return duracion * 0.0343 / 2.0;
}

// Promedio de 5 lecturas para filtrar ruido (olas, lluvia, basura flotando)
float medirDistanciaFiltrada() {
  float suma = 0;
  int validas = 0;
  for (int i = 0; i < 5; i++) {
    float d = medirDistanciaCm();
    if (d > 0) { suma += d; validas++; }
    delay(50);
  }
  return validas ? suma / validas : -1;
}

const char* calcularEstado(float pct) {
  if (pct >= 70) return "rojo";
  if (pct >= 40) return "amarillo";
  return "verde";
}

void mostrarLeds(const char* estado) {
  digitalWrite(LED_VERDE,    strcmp(estado, "verde") == 0);
  digitalWrite(LED_AMARILLO, strcmp(estado, "amarillo") == 0);
  digitalWrite(LED_ROJO,     strcmp(estado, "rojo") == 0);
}

// ---------- Conectividad ----------
void conectarWiFi() {
  Serial.print("Conectando a WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println(" listo");
}

void conectarMQTT() {
  while (!mqtt.connected()) {
    String clientId = String("alertagye-") + SENSOR_ID + "-" + String(random(0xffff), HEX);
    Serial.print("Conectando a MQTT... ");
    if (mqtt.connect(clientId.c_str())) {
      Serial.println("listo");
    } else {
      Serial.print("falló, rc=");
      Serial.println(mqtt.state());
      delay(2000);
    }
  }
}

// ---------- Programa principal ----------
void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);

  conectarWiFi();
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
}

void loop() {
  if (!mqtt.connected()) conectarMQTT();
  mqtt.loop();

  if (millis() - ultimoEnvio < INTERVALO_MS) return;
  ultimoEnvio = millis();

  float distancia = medirDistanciaFiltrada();
  if (distancia < 0) {
    Serial.println("Lectura inválida, se omite");
    return;
  }

  float nivel = ALTURA_SENSOR - distancia;
  if (nivel < 0) nivel = 0;
  float pct = nivel / ALTURA_CRITICA * 100.0;
  if (pct > 100) pct = 100;

  const char* estado = calcularEstado(pct);
  mostrarLeds(estado);

  char payload[200];
  snprintf(payload, sizeof(payload),
    "{\"sensor_id\":\"%s\",\"barrio\":\"%s\",\"distancia_cm\":%.1f,"
    "\"nivel_cm\":%.1f,\"pct\":%.0f,\"estado\":\"%s\"}",
    SENSOR_ID, BARRIO, distancia, nivel, pct, estado);

  mqtt.publish(topico.c_str(), payload);
  Serial.print(topico);
  Serial.print(" -> ");
  Serial.println(payload);
}
