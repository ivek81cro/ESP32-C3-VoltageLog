# ESP32-C3 Voltage Logger

A real-time voltage monitoring system for ESP32-C3 microcontroller with WiFi connectivity and Firebase Realtime Database integration.

## Features

- **Real-time Voltage Monitoring**: ADC readings from GPIO4 (ADC1 channel)
- **WiFi Configuration**: Web-based interface for easy WiFi setup
- **Firebase Integration**: Automatic data upload to Firebase Realtime Database
- **Authentication**: Anonymous Firebase authentication with ID tokens
- **Data Management**: Automatic retention of latest 20 voltage readings
- **Access Point Mode**: Fallback AP mode for initial configuration
- **Web Server**: Built-in async web server for WiFi and configuration

## Hardware

- ESP32-C3 Development Board
- Voltage Sensor Module (5:1 voltage divider, 0-25V range)
- Connected to GPIO4 (ADC1)

## Project Structure
