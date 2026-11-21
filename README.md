# ESP32-C3 Voltage Logger

A real-time voltage monitoring system for ESP32-C3 microcontroller with WiFi connectivity and Firebase Realtime Database integration.

## Features

- **Real-time Voltage Monitoring**: ADC readings from GPIO4 (ADC1 channel)
- **WiFi Configuration**: Web-based interface for easy WiFi setup
- **Firebase Integration**: Automatic data upload to Firebase Realtime Database
- **Authentication**: Firebase email/password authentication with ID tokens
- **Data Management**: Legacy voltage readings use 20 rotating slots; the new history has no implemented retention policy
- **Access Point Mode**: Fallback AP mode for initial configuration
- **Web Server**: Built-in async web server for WiFi and configuration

## Hardware

- ESP32-C3 Development Board
- Voltage Sensor Module (5:1 voltage divider; actual safe input range must be verified)
- Connected to GPIO4 (ADC1)

## Project Structure


## Development phase 2

Measurements run approximately every 10 seconds, while a separate task handles Firebase requests and retry backoff. Wi-Fi loss does not start the access point. The `/status` endpoint reports Wi-Fi, token presence, the last attempt and the last successful write; token presence does not confirm backend availability or a device claim. `/scan` uses asynchronous scanning and `/config` accepts POST requests. See the local [phase 2 notes](docs-local/FAZA_2_NALAZI.md) for details and outstanding checks.

The configuration access point and web interface are unauthenticated. Use only an isolated, trusted network and verify backups before changing the firmware, as described in the local [phase 0 and 1 notes](docs-local/FAZE_0_1_NALAZI.md).

## Shared schema transition · 21 November 2026

The firmware registers the voltage sensor and writes measurement time in milliseconds, `value`, and `rawValue` to `readings/<deviceId>/voltage/<readingId>`. The legacy `voltageReadings` write remains active for existing clients. The user confirmed that the new write works after updating Firebase rules; sensor registration was also observed in the database. The diagnostic firmware was built and uploaded to the ESP32-C3 on COM6.

Only one failed new reading is retained in RAM; there is no persistent offline queue or `latestReadings`. The two writes are independent and use different device identifiers and timestamp semantics. Android compatibility and historical migration remain unverified. Details, access-rule limitations, and remaining acceptance tests are in the [shared schema](SCHEMA_MIKROKONTROLER.md) and [local pilot plan](PLAN_PILOTA.md).
