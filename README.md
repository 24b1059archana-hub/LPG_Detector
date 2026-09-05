# LPG Gas Leakage Detector using 8051 Microcontroller

This project is a real-time **LPG Gas Leakage Detection and Alert System** developed using the 8051 microcontroller. The system detects LPG gas leakage and immediately activates safety measures including buzzer alert, exhaust fan, and SMS notification.

## Features

- Real-time LPG gas detection using MQ-6 sensor
- Buzzer alert for local warning
- Automatic Exhaust Fan activation to clear leaked gas
- SMS alert sent to registered mobile number via GSM module
- Designed and simulated in **Proteus**
- Programmed using **Keil µVision** (Embedded C)

## Components Used

- 8051 Microcontroller (AT89C51 / AT89S52)
- MQ-6 LPG Gas Sensor
- GSM Module (SIM900A / SIM800)
- Buzzer
- DC Motor (used as Exhaust Fan)
- 16x2 LCD (optional)
- Power Supply

## Software Tools

- **Keil µVision** – Embedded C programming
- **Proteus** – Circuit design and simulation

## Working

1. The MQ-6 sensor continuously monitors LPG concentration in the air.
2. When gas level exceeds the threshold:
   - Buzzer is activated
   - Exhaust fan is turned ON
   - SMS alert is sent through GSM module
3. System returns to normal when gas level becomes safe.
