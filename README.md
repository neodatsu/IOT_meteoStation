# MeteoStation

Station meteorologique IoT basee sur un ESP32.

## Capteurs

- **DHT11** - Temperature et humidite
- **Module de temperature NTC** - Temperature de precision (module autonome 4 broches avec thermistance)
- **Module LDR** - Luminosite ambiante

## Materiel

- ESP32 Wemos Lolin32 (wemosbat)
- Capteur DHT11
- Module de temperature NTC (4 broches : VCC, GND, DO, AO)
- Module LDR (3 broches : VCC, GND, Signal)

## Branchements GPIO

| Capteur                  | GPIO |
|--------------------------|------|
| DHT11                    | 27   |
| Module temperature (AO)  | 34   |
| Module LDR (luminosite)  | 35   |

Alimentation des capteurs en **3,3V** via la broche 3V3 de l'ESP32.

### Schema de cablage

```text
                    +-----------+
                    |  ESP32    |
                    |           |
    DHT11           |           |        Module Temp       Module LDR
    +-------+       |           |         +--------+       +-------+
    | VCC   |--+----|3V3        |    +----|VCC     |  +----|VCC    |
    | DATA  |--|----|GPIO 27    |    |    |GND     |--|    |GND    |
    | GND   |--|-+--|GND        |    | +--|DO (nc) |  | +--|SIG    |
    +-------+  | |  |           |    | |  |AO      |--|-|  +-------+
               | |  |   GPIO 34 |----+ |  +--------+  | |
               | |  |   GPIO 35 |------|--------------+  |
               | |  |           |      |                  |
               | |  +-----------+      |                  |
               | |                     |                  |
               | +---------------------+------------------+
               +-------------------------------------------+

    Legende :
    nc  = non connecte
    3V3 = alimentation 3,3V (commune a tous les capteurs)
    GND = masse commune
```

## Connexion WiFi

L'ESP32 se connecte automatiquement au WiFi au demarrage et se reconnecte en cas de perte de connexion.

### Configuration des identifiants

Les identifiants (WiFi et MQTT) sont stockes dans `include/credentials.h` qui est **exclu du depot git** (via `.gitignore`).

Pour configurer votre connexion :

1. Copier le fichier exemple :

   ```bash
   cp include/credentials.h.example include/credentials.h
   ```

2. Editer `include/credentials.h` avec vos parametres :

   ```cpp
   #define WIFI_SSID   "votre_ssid"
   #define WIFI_PASS   "votre_mot_de_passe"
   #define MQTT_SERVER "mqtt.example.com"
   #define MQTT_PORT   8883
   #define MQTT_USER   "votre_email@example.com"
   #define MQTT_PASS   "votre_mot_de_passe_mqtt"
   #define MQTT_DEVICE "meteoStation_1"
   ```

### Comportement

- Mode **Station (STA)** : l'ESP32 se connecte a un reseau existant
- Timeout de connexion : **20 secondes**
- Reconnexion automatique en cas de perte du signal
- L'adresse IP est affichee dans les logs serie apres connexion

## MQTT

Les mesures sont publiees sur un broker MQTT en **TLS (port 8883)** a chaque releve.

### Topic

```text
sensors/{MQTT_USER}/{MQTT_DEVICE}
```

### Format du message (JSON)

```json
{
  "timestamp": "2026-02-08T15:30:00+01:00",
  "user": "votre_email@example.com",
  "device": "meteoStation_1",
  "dht_temperature": 20.7,
  "dht_humidity": 52.0,
  "ntc_temperature": 21.1,
  "luminosity": 77.0
}
```

- **timestamp** : heure locale France (CET/CEST) au format ISO 8601, synchronisee via NTP
- Si le DHT11 est en erreur, `dht_temperature` et `dht_humidity` sont a `null`

## Stack technique

- **Framework** : Arduino (via PlatformIO)
- **Plateforme** : Espressif32
- **IDE** : PlatformIO (VSCode)

## Structure du projet

```text
meteoStation/
├── src/           # Code source principal
├── include/       # Headers du projet
├── lib/           # Bibliotheques privees
├── test/          # Tests unitaires
└── platformio.ini # Configuration PlatformIO
```

## Installation

1. Cloner le repository :

   ```bash
   git clone https://github.com/neodatsu/IOT_meteoStation.git
   ```

2. Ouvrir le projet dans VSCode avec l'extension PlatformIO
3. Brancher l'ESP32 en USB
4. Build et upload :

   ```bash
   pio run --target upload
   ```
