# TPMS on ESP32

TPMS (Tire Pressure Monitoring System) on ESP32 is a hardware-software solution for real-time tire pressure and temperature monitoring. The system uses wireless sensors to receive data on UHF 433MHz to an ESP32 microcontroller (via CC1101 UHF tranceiver), which processes and use built-in TWAI controller, transmit it to a vehicle's CAN bus (Chery Tiggo 2, CAN-2) for display on the factory instrument cluster.

## Features

- Available to set splitted pressure for front and rear pair of tires (in config function).
- Available to set temperature for all tires (in config function).
- Three tire state: normal, caution and critical.
- Automatic clear display value through specified time (example, 4 minutes) after receive data from tire sensor.

### Supported tire sensors

- OEM Sensor `802000021AA` (Chery Tiggo 7)
- Autel MX-Sensor, programmed to Chery Tiggo 7 | 07/2018-12/2022 (433MHz `802000021AA`)

### Supported export data methods

- Second CAN bus in Chery Tiggo 2, 2017, values displayed on regular instrument cluster with support TPMS unit (else reprogramming required).

### Supported ESP controllers

- [ESP32-WROOM-32D]
- [ESP32-S3]

### Connection to the vehicle's on-board network

Uses 4-pin connection:

1. Power 12VDC
2. Ground
3. CAN-2 High
4. CAN-2 Low

## Dependencis

- [ESP-IDF](https://idf.espressif.com/).

## License

Distributed under the MIT License. See `LICENSE.md` for more information.

## Contact

Viktor Banko S. - [bankoViktor](https://github.com/bankoViktor) - bankviktor14@gmail.com


[ESP32-WROOM-32D]: https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32d_esp32-wroom-32u_datasheet_en.pdf
[ESP32-S3]: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf