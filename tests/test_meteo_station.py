"""
Tests unitaires pour la logique de la MeteoStation.

Reproduit en Python les calculs effectues dans le firmware ESP32
(equation Beta NTC, conversion LDR, construction du payload MQTT).
"""

import json
import math
import pytest

# --- Constantes du firmware (miroir de main.cpp) ---
R_SERIES = 10000.0
B_COEFF = 3950.0
R_NOMINAL = 1760.0
T_NOMINAL = 25.0
ADC_MAX = 4095.0


def ntc_resistance(raw_adc):
    """Calcul de la resistance de la thermistance depuis la valeur ADC brute."""
    if raw_adc <= 0 or raw_adc >= ADC_MAX:
        raise ValueError(f"Valeur ADC hors limites : {raw_adc}")
    return R_SERIES * raw_adc / (ADC_MAX - raw_adc)


def ntc_temperature(raw_adc):
    """Conversion ADC -> temperature (C) via l'equation Beta."""
    resistance = ntc_resistance(raw_adc)
    temp_k = 1.0 / (
        1.0 / (T_NOMINAL + 273.15) + math.log(resistance / R_NOMINAL) / B_COEFF
    )
    return temp_k - 273.15


def ldr_percentage(raw_adc):
    """Conversion ADC -> luminosite en pourcentage."""
    return raw_adc * 100.0 / ADC_MAX


def adc_average(samples):
    """Moyenne d'une liste d'echantillons ADC."""
    if not samples:
        raise ValueError("Liste d'echantillons vide")
    return sum(samples) / len(samples)


def build_payload(timestamp, user, device, dht_temp, dht_humidity, ntc_temp, luminosity):
    """Construit le payload JSON identique au firmware."""
    payload = {
        "timestamp": timestamp,
        "user": user,
        "device": device,
        "dht_temperature": dht_temp,
        "dht_humidity": dht_humidity,
        "ntc_temperature": ntc_temp,
        "luminosity": luminosity,
    }
    return payload


# =============================================================================
# Tests NTC
# =============================================================================

class TestNtcTemperature:
    """Tests du calcul de temperature NTC via l'equation Beta."""

    def test_known_reference_point(self):
        """A R_NOMINAL, la temperature doit etre T_NOMINAL (25 C)."""
        # raw tel que R_SERIES * raw / (4095 - raw) = R_NOMINAL
        # => raw = R_NOMINAL * 4095 / (R_SERIES + R_NOMINAL)
        raw = R_NOMINAL * ADC_MAX / (R_SERIES + R_NOMINAL)
        temp = ntc_temperature(raw)
        assert temp == pytest.approx(T_NOMINAL, abs=0.01)

    def test_higher_temperature(self):
        """Une valeur ADC plus elevee (resistance plus haute) donne une temperature plus basse."""
        raw_low = 1000
        raw_high = 3000
        assert ntc_temperature(raw_low) > ntc_temperature(raw_high)

    def test_resistance_calculation(self):
        """Verifie le calcul de resistance du pont diviseur."""
        # raw = 2048 (milieu de l'ADC) => R = 10000 * 2048 / 2047 ~ 10004.9
        resistance = ntc_resistance(2048)
        assert resistance == pytest.approx(R_SERIES * 2048 / (ADC_MAX - 2048), abs=0.1)

    def test_adc_zero_raises(self):
        """ADC = 0 doit lever une erreur (division par zero evitee)."""
        with pytest.raises(ValueError):
            ntc_temperature(0)

    def test_adc_max_raises(self):
        """ADC = 4095 doit lever une erreur (division par zero)."""
        with pytest.raises(ValueError):
            ntc_temperature(ADC_MAX)

    def test_temperature_range_realistic(self):
        """Pour des valeurs ADC typiques, la temperature doit etre dans une plage realiste."""
        for raw in range(100, 3500, 200):
            temp = ntc_temperature(raw)
            assert -40 < temp < 150, f"Temperature {temp} C hors plage pour ADC={raw}"


# =============================================================================
# Tests LDR
# =============================================================================

class TestLdr:
    """Tests de la conversion LDR en pourcentage."""

    def test_ldr_zero(self):
        """ADC 0 = 0% de luminosite."""
        assert ldr_percentage(0) == pytest.approx(0.0)

    def test_ldr_max(self):
        """ADC 4095 = 100% de luminosite."""
        assert ldr_percentage(ADC_MAX) == pytest.approx(100.0)

    def test_ldr_mid(self):
        """ADC milieu = ~50% de luminosite."""
        assert ldr_percentage(2048) == pytest.approx(50.0, abs=0.1)

    def test_ldr_range(self):
        """Le pourcentage est toujours entre 0 et 100."""
        for raw in range(0, 4096, 100):
            pct = ldr_percentage(raw)
            assert 0.0 <= pct <= 100.0


# =============================================================================
# Tests moyennage ADC
# =============================================================================

class TestAdcAverage:
    """Tests du moyennage des echantillons ADC."""

    def test_single_sample(self):
        """Un seul echantillon retourne sa valeur."""
        assert adc_average([2048]) == 2048

    def test_uniform_samples(self):
        """Des echantillons identiques retournent cette valeur."""
        assert adc_average([1000] * 20) == 1000

    def test_known_average(self):
        """Moyenne de valeurs connues."""
        assert adc_average([1000, 2000, 3000]) == pytest.approx(2000.0)

    def test_nb_samples_default(self):
        """Le firmware utilise 20 echantillons."""
        samples = [2000 + i for i in range(20)]
        expected = sum(samples) / 20
        assert adc_average(samples) == pytest.approx(expected)

    def test_empty_raises(self):
        """Une liste vide doit lever une erreur."""
        with pytest.raises(ValueError):
            adc_average([])


# =============================================================================
# Tests payload MQTT
# =============================================================================

class TestMqttPayload:
    """Tests de la construction du payload MQTT JSON."""

    def test_payload_structure(self):
        """Le payload contient tous les champs attendus."""
        payload = build_payload(
            "2026-02-08T15:30:00+01:00", "user@example.com", "meteoStation_1",
            20.7, 52.0, 21.1, 77.0,
        )
        assert set(payload.keys()) == {
            "timestamp", "user", "device",
            "dht_temperature", "dht_humidity",
            "ntc_temperature", "luminosity",
        }

    def test_payload_values(self):
        """Les valeurs du payload correspondent aux entrees."""
        payload = build_payload(
            "2026-02-08T15:30:00+01:00", "user@example.com", "meteoStation_1",
            20.7, 52.0, 21.1, 77.0,
        )
        assert payload["dht_temperature"] == 20.7
        assert payload["dht_humidity"] == 52.0
        assert payload["ntc_temperature"] == 21.1
        assert payload["luminosity"] == 77.0
        assert payload["device"] == "meteoStation_1"

    def test_payload_json_serializable(self):
        """Le payload doit etre serialisable en JSON."""
        payload = build_payload(
            "2026-02-08T15:30:00+01:00", "user@example.com", "meteoStation_1",
            20.7, 52.0, 21.1, 77.0,
        )
        json_str = json.dumps(payload)
        parsed = json.loads(json_str)
        assert parsed == payload

    def test_payload_dht_null(self):
        """Si le DHT11 est en erreur, les valeurs sont None (null en JSON)."""
        payload = build_payload(
            "2026-02-08T15:30:00+01:00", "user@example.com", "meteoStation_1",
            None, None, 21.1, 77.0,
        )
        assert payload["dht_temperature"] is None
        assert payload["dht_humidity"] is None
        json_str = json.dumps(payload)
        assert '"dht_temperature": null' in json_str

    def test_payload_timestamp_format(self):
        """Le timestamp doit etre au format ISO 8601 avec timezone."""
        payload = build_payload(
            "2026-02-08T15:30:00+01:00", "user@example.com", "meteoStation_1",
            20.7, 52.0, 21.1, 77.0,
        )
        ts = payload["timestamp"]
        assert "T" in ts
        assert "+" in ts or "-" in ts[10:]  # timezone offset

    def test_mqtt_topic_format(self):
        """Le topic MQTT suit le format sensors/{user}/{device}."""
        user = "user@example.com"
        device = "meteoStation_1"
        topic = f"sensors/{user}/{device}"
        assert topic == "sensors/user@example.com/meteoStation_1"
