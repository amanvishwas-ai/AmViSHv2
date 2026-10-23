# AmViSH 2.0 pinout reference

This table summarizes the supplied `ARDUINO_CODE.ino`, ESP32 bridge sketch, and `AMVISH CONNECTIONS.xlsx`. The firmware is the source of truth for what the current program actually uses; the workbook also lists peripherals that are not implemented.

## Arduino connections in current firmware

| Part | Signal | Arduino pin | Notes |
|---|---|---:|---|
| Right-hand servo | Signal | D9 | `servoRight` |
| Left-hand servo | Signal | D10 | `servoLeft` |
| Head servo | Signal | D11 | `servoHead` |
| Motor driver, motor A | IN1 | D7 | |
| Motor driver, motor A | IN2 | D4 | |
| Motor driver, motor A | ENA | D5 / PWM | |
| Motor driver, motor B | IN3 | D8 | |
| Motor driver, motor B | IN4 | D12 | |
| Motor driver, motor B | ENB | D6 / PWM | |
| ESP32 to Arduino UART | RX | D2 | `SoftwareSerial espSerial(2, -1)`; 9600 baud |

The ESP32 bridge sends UART TX on GPIO 15; its UART RX is disabled. Connect grounds together. Confirm signal levels and board pin capabilities for your exact hardware.

## Workbook-only connections not implemented in the supplied Arduino firmware

| Part | Signal | Workbook pin | Current status |
|---|---|---:|---|
| Ultrasonic sensor | TRIG | D2 | Not implemented; conflicts with ESP32 serial RX on D2 |
| Ultrasonic sensor | ECHO | D3 | Not implemented |
| I²C display | SDA | A4 | Not implemented |
| I²C display | SCL | A5 | Not implemented |

Do not connect the ultrasonic sensor’s TRIG to D2 while using the supplied serial configuration. Reassign a pin and update the firmware before adding this sensor. The firmware includes `Wire.h` but does not initialize or write to a display.

## ESP32 bridge network

- Wi-Fi credentials are local-only in `secrets.h`.
- UDP discovery port: `4210` (`AMVISH_PING` / `AMVISH_PONG`).
- HTTP server: port `80`.
- Action endpoint: `POST /ACT` with a JSON body containing an `act` string.
- UART toward Arduino: GPIO 15 TX, 9600 baud, 8N1.

The HTTP endpoint has no authentication. Keep it on a trusted private network.
