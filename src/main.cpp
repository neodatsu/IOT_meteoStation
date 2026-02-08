/**
 * MeteoStation - Station meteorologique IoT basee sur ESP32
 *
 * Capteurs :
 *   - DHT11 : temperature et humidite (GPIO 27)
 *   - Module NTC : temperature analogique via thermistance (GPIO 34)
 *   - Module LDR : luminosite ambiante (GPIO 35)
 *
 * Connexion WiFi automatique avec reconnexion en cas de perte.
 * Les identifiants WiFi sont dans include/credentials.h (non versionne).
 */

#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>
#include <math.h>
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

DHT dht(DHT_PIN, DHT_TYPE);

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

void setup() {
  Serial.begin(115200);
  dht.begin();
  analogSetAttenuation(ADC_11db);  // Plage 0-3.3V pour l'ADC
  delay(2000);
  Serial.println("=== MeteoStation demarree ===");
  connectWiFi();
}

void loop() {
  // Reconnexion auto si deconnecte
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi perdu, reconnexion...");
    connectWiFi();
  }

  // --- DHT11 : temperature et humidite ---
  float humidity = dht.readHumidity();
  float dhtTemp = dht.readTemperature();

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

  if (isnan(humidity) || isnan(dhtTemp)) {
    Serial.println("DHT11           : erreur de lecture");
  } else {
    Serial.printf("DHT11           : %.1f C | %.1f %%\n", dhtTemp, humidity);
  }

  Serial.printf("Temperature NTC : %.1f C\n", ntcTemp);
  Serial.printf("Luminosite      : %.0f %%\n", ldrPct);
  Serial.println();

  delay(READ_INTERVAL);
}
