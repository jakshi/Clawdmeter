# Component datasheets

All hosted by Waveshare/Espressif. Fetch on demand; not vendored.

## On-board components (AMOLED-1.8)

| Component | Role | Datasheet |
|-----------|------|-----------|
| CST816(D) | Touch controller (V2 panel) | https://files.waveshare.com/wiki/common/CST816D_datasheet_En_V1.3.pdf |
| FT3168 | Touch controller (V1 panel) | https://files.waveshare.com/wiki/common/FT3168.pdf |
| CO5300 | AMOLED driver (V2 panel) | https://files.waveshare.com/wiki/common/CO5300_Datasheet_V0.00.pdf |
| SH8601 | AMOLED driver (V1 panel) | https://files.waveshare.com/wiki/common/SH8601A0_DataSheet_Preliminary_V0.0_UCS_191107_1.pdf |
| AXP2101 | PMU / charger / PWRKEY | https://files.waveshare.com/wiki/common/X-power-AXP2101_SWcharge_V1.0.pdf |
| QMI8658(C) | IMU (accel/gyro) | https://files.waveshare.com/wiki/common/QMI8658C.pdf |
| PCF85063(A) | RTC | https://files.waveshare.com/wiki/common/PCF85063A.pdf |
| ES8311 | Audio codec | https://files.waveshare.com/wiki/common/ES8311.DS.pdf (+ user guide: https://files.waveshare.com/wiki/common/ES8311.user.Guide.pdf) |

Note: the XCA9554/PCA9554 IO expander datasheet isn't on Waveshare's page — use NXP/TI PCA9554 directly.

## Board-level

- Schematic: https://files.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.8/ESP32-S3-Touch-AMOLED-1.8.pdf
- 3D: https://files.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.8/ESP32-S3-Touch-AMOLED-1.8-3D.zip

## Espressif (large)

- ESP32-S3 TRM: https://documentation.espressif.com/esp32-s3_technical_reference_manual_en.pdf
- ESP32-S3 Datasheet: https://documentation.espressif.com/esp32-s3_datasheet_en.pdf

## Most relevant right now

- **CST816D** — register map for the "dead touch" fix (interrupt-mode `0xFA`, sleep-mode `0xE5`, gesture/finger regs `0x01`–`0x06`).
- **AXP2101** — PWRKEY press on/off timing, charger config, `setSysPowerDownVoltage`.
- **Schematic** — confirm whether the PWR button reaches the AXP PWRKEY, EXIO4, or both.
