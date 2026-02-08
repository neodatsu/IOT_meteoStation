# MeteoStation

Station meteorologique IoT basee sur un ESP32.

## Capteurs

- **Module de temperature NTC** - Temperature de precision (module autonome 4 broches avec thermistance)

## Materiel

- ESP32 Wemos Lolin32 (wemosbat)
- Module de temperature NTC (4 broches : VCC, GND, DO, AO)

## Branchements GPIO

| Capteur                  | GPIO |
|--------------------------|------|
| Module temperature (AO)  | 34   |

Alimentation du capteur en **3,3V** via la broche 3V3 de l'ESP32.

### Schema de cablage

```
                    +-----------+
                    |  ESP32    |
                    |           |        Module Temp
                    |           |         +--------+
                    |3V3--------|----+----|VCC     |
                    |GND--------|--+-|----|GND     |
                    |           |  | |    |DO (nc) |
                    |   GPIO 34 |--|-|----|AO      |
                    |           |  | |    +--------+
                    +-----------+  | |
                                   | |
                    GND commun ----+ |
                    3V3 commun ------+

    Legende :
    nc  = non connecte
    3V3 = alimentation 3,3V
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
