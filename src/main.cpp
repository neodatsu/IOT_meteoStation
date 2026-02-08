/**
 * MeteoStation - DHT11 + Module temperature NTC + LDR
 */

#include <Arduino.h>
#include <DHT.h>
#include <math.h>

// --- Configuration des pins ---
#define DHT_PIN     27    // Data DHT11
#define TEMP_AO_PIN 34    // Sortie analogique du module NTC
#define LDR_PIN     35    // Signal du module LDR LDR

// --- Parametres DHT11 ---
#define DHT_TYPE DHT11

// --- Parametres de lecture ---
#define READ_INTERVAL 10000
#define NB_SAMPLES 20

// --- Parametres de la thermistance NTC (calibres pour le module) ---
#define R_SERIES   10000.0
#define B_COEFF    3950.0
#define R_NOMINAL  1760.0
#define T_NOMINAL  25.0

DHT dht(DHT_PIN, DHT_TYPE);

/**
 * Lecture analogique moyennee pour lisser le bruit de l'ADC ESP32.
 */
int analogReadAvg(int pin) {
  long sum = 0;
  for (int i = 0; i < NB_SAMPLES; i++) {
    sum += analogRead(pin);
    delay(5);
  }
  return sum / NB_SAMPLES;
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  analogSetAttenuation(ADC_11db);
  delay(2000);
  Serial.println("=== MeteoStation demarree ===");
}

void loop() {
  // --- DHT11 ---
  float humidity = dht.readHumidity();
  float dhtTemp = dht.readTemperature();

  // --- Module NTC ---
  int raw = analogReadAvg(TEMP_AO_PIN);
  float resistance = R_SERIES * raw / (4095.0 - raw);
  float tempK = 1.0 / (1.0 / (T_NOMINAL + 273.15) + log(resistance / R_NOMINAL) / B_COEFF);
  float ntcTemp = tempK - 273.15;

  // --- Affichage ---
  Serial.flush();
  Serial.println("--- Releve capteurs ---");

  if (isnan(humidity) || isnan(dhtTemp)) {
    Serial.println("DHT11         : erreur de lecture");
  } else {
    Serial.printf("DHT11         : %.1f C | %.1f %%\n", dhtTemp, humidity);
  }

  Serial.printf("Temperature NTC : %.1f C\n", ntcTemp);

  // --- LDR ---
  int ldrValue = analogReadAvg(LDR_PIN);
  float ldrPct = ldrValue * 100.0 / 4095.0;
  Serial.printf("Luminosite      : %.0f %%\n", ldrPct);
  Serial.println();

  delay(READ_INTERVAL);
}
