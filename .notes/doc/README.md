# Reference docs — ESP32-S3-Touch-AMOLED-1.8

Index of external sources gathered while debugging the AMOLED-1.8 V2 board
(CO5300 + CST816). Links are upstream; nothing large is vendored here.

- [`datasheets.md`](datasheets.md) — component + Espressif datasheets (CST816, AXP2101, CO5300, …).
- [`reference-repos.md`](reference-repos.md) — official Waveshare repo (how to clone) + Community Showcase projects.
- [`findings.md`](findings.md) — what the references told us vs. how our firmware differs (PWR, power-off, CST816 touch).
- [`diagnostic-scaffolding.md`](diagnostic-scaffolding.md) — the removed wake-after-power-off instrumentation, restorable for the 20 h brownout.
- [`peer-exchange.md`](peer-exchange.md) — cross-agent notes with a peer on the same V2 board (PEK + CST816-touch leads).

## Primary sources

- Waveshare wiki — Resources & Documents:
  https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.8/Resources-And-Documents
- Waveshare wiki — Arduino setup:
  https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.8/Development-Environment-Setup-Arduino
- Official example repo:
  https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8
- Schematic (PDF):
  https://files.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.8/ESP32-S3-Touch-AMOLED-1.8.pdf

## Espressif (large PDFs — link only, fetch sections as needed)

- ESP32-S3 Technical Reference Manual:
  https://documentation.espressif.com/esp32-s3_technical_reference_manual_en.pdf
- ESP32-S3 Datasheet:
  https://documentation.espressif.com/esp32-s3_datasheet_en.pdf
