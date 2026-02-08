# MeteoStation

Station meteorologique IoT basee sur un ESP32.

## Capteurs

- **DHT11** - Temperature et humidite
- **Module de temperature NTC** - Temperature de precision (module autonome 4 broches avec thermistance)
- **Module LDR (KY-018)** - Luminosite ambiante

## Materiel

- ESP32 Wemos Lolin32 (wemosbat)
- Capteur DHT11
- Module de temperature NTC (4 broches : VCC, GND, DO, AO)
- Module LDR KY-018 (3 broches : VCC, GND, Signal)

## Branchements GPIO

| Capteur                  | GPIO |
|--------------------------|------|
| DHT11                    | 27   |
| Module temperature (AO)  | 34   |
| Module LDR (luminosite)  | 35   |

Alimentation des capteurs en **3,3V** via la broche 3V3 de l'ESP32.

### Schema de cablage

```
                    +-----------+
                    |  ESP32    |
                    |           |
    DHT11           |           |        Module Temp       KY-018
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

## Stack technique

- **Framework** : Arduino (via PlatformIO)
- **Plateforme** : Espressif32
- **IDE** : PlatformIO (VSCode)

## Structure du projet

```
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
