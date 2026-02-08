/**
 * MeteoStation - Station meteorologique IoT basee sur ESP32
 *
 * Capteurs :
 *   - DHT11 : temperature et humidite (GPIO 27)
 *   - Module NTC : temperature analogique via thermistance (GPIO 34)
 *   - Module LDR : luminosite ambiante (GPIO 35)
 *
 * Connexion WiFi automatique avec reconnexion en cas de perte.
 * Synchronisation NTP (fuseau France CET/CEST).
 * Publication des mesures sur un serveur MQTT en TLS (port 8883).
 * Les identifiants sont dans include/credentials.h (non versionne).
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <math.h>
#include <time.h>
#include "credentials.h"

// --- Configuration des pins ---
#define DHT_PIN     27    // Data DHT11
#define TEMP_AO_PIN 34    // Sortie analogique du module NTC
#define LDR_PIN     35    // Signal du module LDR

// --- Parametres DHT11 ---
#define DHT_TYPE DHT11

// --- Parametres de lecture ---
#define READ_INTERVAL 10000  // Intervalle entre chaque releve (ms)
#define NB_SAMPLES 20        // Nombre d'echantillons pour le moyennage ADC

// --- Parametres de la thermistance NTC (calibres pour le module) ---
// Equation Beta (Steinhart-Hart simplifiee) :
//   1/T = 1/T0 + (1/B) * ln(R/R0)
// R_SERIES   : resistance en serie sur le module (10k)
// B_COEFF    : coefficient Beta de la thermistance
// R_NOMINAL  : resistance de la thermistance a T_NOMINAL (calibree)
// T_NOMINAL  : temperature de reference (25 C)
#define R_SERIES   10000.0
#define B_COEFF    3950.0
#define R_NOMINAL  1760.0
#define T_NOMINAL  25.0

// --- NTP : fuseau horaire France (CET/CEST) ---
#define NTP_SERVER "pool.ntp.org"
// CET = UTC+1, CEST = UTC+2 (dernier dimanche de mars -> dernier dimanche d'octobre)
#define TZ_FRANCE  "CET-1CEST,M3.5.0,M10.5.0/3"

// --- MQTT : topic construit a partir des credentials ---
// Format : meteo/{MQTT_USER}/{MQTT_DEVICE}
#define MQTT_TOPIC "sensors/" MQTT_USER "/" MQTT_DEVICE

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClientSecure espClient;
PubSubClient mqtt(espClient);

/**
 * Lecture analogique moyennee pour lisser le bruit de l'ADC ESP32.
 * Effectue NB_SAMPLES lectures espacees de 5ms et retourne la moyenne.
 */
int analogReadAvg(int pin) {
  long sum = 0;
  for (int i = 0; i < NB_SAMPLES; i++) {
    sum += analogRead(pin);
    delay(5);
  }
  return sum / NB_SAMPLES;
}

/**
 * Retourne le timestamp actuel au format ISO 8601 (ex: 2026-02-08T15:30:00+01:00).
 * Necessite que le NTP soit synchronise.
 */
String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "null";
  }
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", &timeinfo);
  // Inserer ':' dans l'offset timezone (+0100 -> +01:00)
  String ts(buf);
  if (ts.length() >= 24) {
    ts = ts.substring(0, ts.length() - 2) + ":" + ts.substring(ts.length() - 2);
  }
  return ts;
}

/**
 * Connexion au WiFi avec timeout de 20 secondes.
 * Retourne true si connecte, false sinon.
 */
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.printf("Connexion WiFi a %s...\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Attente de connexion (40 x 500ms = 20s max)
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Connecte ! IP : %s\n", WiFi.localIP().toString().c_str());
    return true;
  } else {
    Serial.printf("Echec connexion (status: %d)\n", WiFi.status());
    return false;
  }
}

/**
 * Connexion au broker MQTT avec authentification.
 * Retente 3 fois en cas d'echec.
 */
bool connectMQTT() {
  for (int i = 0; i < 3 && !mqtt.connected(); i++) {
    Serial.printf("Connexion MQTT a %s...\n", MQTT_SERVER);
    if (mqtt.connect(MQTT_DEVICE, MQTT_USER, MQTT_PASS)) {
      Serial.println("MQTT connecte !");
      return true;
    }
    Serial.printf("Echec MQTT (rc=%d), nouvelle tentative...\n", mqtt.state());
    delay(2000);
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  analogSetAttenuation(ADC_11db);  // Plage 0-3.3V pour l'ADC
  delay(2000);
  Serial.println("=== MeteoStation demarree ===");

  connectWiFi();

  // Synchronisation NTP (fuseau France)
  configTzTime(TZ_FRANCE, NTP_SERVER);
  Serial.println("Synchronisation NTP...");
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 10000)) {
    Serial.printf("Heure : %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  } else {
    Serial.println("Echec synchronisation NTP");
  }

  // Configuration MQTT (TLS sans verification de certificat)
  espClient.setInsecure();
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  connectMQTT();
}

void loop() {
  // Reconnexion WiFi auto si deconnecte
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi perdu, reconnexion...");
    connectWiFi();
  }

  // Reconnexion MQTT auto si deconnecte
  if (!mqtt.connected()) {
    connectMQTT();
  }
  mqtt.loop();

  // --- DHT11 : temperature et humidite ---
  float humidity = dht.readHumidity();
  float dhtTemp = dht.readTemperature();
  bool dhtOk = !isnan(humidity) && !isnan(dhtTemp);

  // --- Module NTC : calcul de la temperature via equation Beta ---
  int raw = analogReadAvg(TEMP_AO_PIN);
  // Calcul de la resistance de la thermistance a partir du pont diviseur
  float resistance = R_SERIES * raw / (4095.0 - raw);
  // Conversion en temperature via l'equation Beta
  float tempK = 1.0 / (1.0 / (T_NOMINAL + 273.15) + log(resistance / R_NOMINAL) / B_COEFF);
  float ntcTemp = tempK - 273.15;

  // --- LDR : luminosite en pourcentage ---
  int ldrValue = analogReadAvg(LDR_PIN);
  float ldrPct = ldrValue * 100.0 / 4095.0;

  // --- Affichage des releves ---
  Serial.flush();
  Serial.println("--- Releve capteurs ---");

  if (!dhtOk) {
    Serial.println("DHT11           : erreur de lecture");
  } else {
    Serial.printf("DHT11           : %.1f C | %.1f %%\n", dhtTemp, humidity);
  }

  Serial.printf("Temperature NTC : %.1f C\n", ntcTemp);
  Serial.printf("Luminosite      : %.0f %%\n", ldrPct);

  // --- Publication MQTT au format JSON ---
  String ts = getTimestamp();

  char payload[512];
  if (dhtOk) {
    snprintf(payload, sizeof(payload),
      "{\"timestamp\":\"%s\","
      "\"user\":\"%s\","
      "\"device\":\"%s\","
      "\"dht_temperature\":%.1f,"
      "\"dht_humidity\":%.1f,"
      "\"ntc_temperature\":%.1f,"
      "\"luminosity\":%.1f}",
      ts.c_str(), MQTT_USER, MQTT_DEVICE,
      dhtTemp, humidity, ntcTemp, ldrPct);
  } else {
    snprintf(payload, sizeof(payload),
      "{\"timestamp\":\"%s\","
      "\"user\":\"%s\","
      "\"device\":\"%s\","
      "\"dht_temperature\":null,"
      "\"dht_humidity\":null,"
      "\"ntc_temperature\":%.1f,"
      "\"luminosity\":%.1f}",
      ts.c_str(), MQTT_USER, MQTT_DEVICE,
      ntcTemp, ldrPct);
  }

  if (mqtt.connected()) {
    if (mqtt.publish(MQTT_TOPIC, payload)) {
      Serial.printf("MQTT publie sur %s\n", MQTT_TOPIC);
    } else {
      Serial.println("Echec publication MQTT");
    }
  } else {
    Serial.println("MQTT non connecte, message non envoye");
  }

  Serial.println();
  delay(READ_INTERVAL);
}
