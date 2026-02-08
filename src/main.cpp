/**
 * MeteoStation - Module temperature NTC
 *
 * Lecture de la temperature via un module thermistance NTC
 * connecte sur la sortie analogique (AO) du module.
 * La conversion utilise l'equation Beta (Steinhart-Hart simplifiee).
 */

#include <Arduino.h>
#include <math.h>

// --- Configuration des pins ---
#define TEMP_AO_PIN 34        // Sortie analogique du module NTC

// --- Parametres de lecture ---
#define READ_INTERVAL 2000    // Intervalle entre les releves (ms)
#define NB_SAMPLES 20         // Nombre de lectures pour le moyennage ADC

// --- Parametres de la thermistance NTC ---
#define R_SERIES   10000.0    // Resistance serie du diviseur de tension (ohms)
#define B_COEFF    3950.0     // Coefficient Beta de la thermistance
#define R_NOMINAL  1760.0     // Resistance nominale calibree (ohms)
#define T_NOMINAL  25.0       // Temperature de reference (C)

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

void setup() {
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);  // Plage ADC 0-3.3V
  delay(2000);                     // Stabilisation du capteur
  Serial.println("=== MeteoStation demarree ===");
}

void loop() {
  int raw = analogReadAvg(TEMP_AO_PIN);

  // Calcul de la resistance de la thermistance via le diviseur de tension
  float resistance = R_SERIES * raw / (4095.0 - raw);

  // Conversion resistance -> temperature via l'equation Beta
  float tempK = 1.0 / (1.0 / (T_NOMINAL + 273.15) + log(resistance / R_NOMINAL) / B_COEFF);
  float tempC = tempK - 273.15;

  Serial.printf("Temperature : %.1f C\n", tempC);

  delay(READ_INTERVAL);
}
