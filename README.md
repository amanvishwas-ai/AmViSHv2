# AmViSH 2.0 — embodied robotics experiment

AmViSH 2.0 is an experimental robot that connects a laptop-based voice loop to an ESP32 and an Arduino-controlled mobile body. The laptop listens for speech, transcribes it locally with Whisper, asks a Groq-hosted language model for a constrained action and response, sends the action to the ESP32 over the local network, and speaks the response with the laptop’s text-to-speech engine.

This repository records a prototype, not a production or safety-certified robot. The supplied firmware and wiring sheet are preserved as separate sketches and reference material; see [known limitations](#known-limitations) before wiring or powering the robot.

## System overview

```text
Microphone → laptop audio capture → local Whisper transcription
           → Groq chat completion → {ACT, SAY}
           ├─ ACT → HTTP on local Wi-Fi → ESP32 bridge → UART → Arduino → motors / servos
           └─ SAY → laptop text-to-speech

ESP32 discovery: laptop broadcasts AMVISH_PING over UDP/4210; ESP32 replies AMVISH_PONG.
```

The transcribed text is sent to Groq for inference. Audio capture and Whisper transcription happen on the laptop. Speech output uses `pyttsx3` and the host operating system’s installed voice.

## Repository layout

- `laptop/` — audio capture, Whisper STT, Groq decision logic, TTS, UDP discovery, and main loop.
- `firmware/esp32_cam_act_bridge/` — the ESP32 sketch that implements UDP discovery and the HTTP `/ACT` endpoint used by `laptop/main.py`. The camera is not initialized by this sketch.
- `firmware/ARDUINO_CODE/` — Arduino motor and servo controller.
- `firmware/ESP32_main/` — an alternate status/discovery sketch. It does **not** implement `/ACT`; it is not the action bridge required by the laptop program.
- `hardware/amvish-2.0-connections.xlsx` — supplied wiring reference.
- `hardware/PINOUT.md` — readable pin summary and known differences between the wiring sheet and the current firmware.

## Hardware

The exact board revisions, motor driver model, power supply, and camera module were not identified in the supplied files. The current code’s pin assignments are summarized in [hardware/PINOUT.md](hardware/PINOUT.md). Confirm voltage compatibility and wiring against the datasheets for your actual components before connecting power.

The action path expects:

1. An ESP32 running `firmware/esp32_cam_act_bridge/esp32_cam_act_bridge.ino`.
2. The ESP32 UART TX (GPIO 15 in this sketch) connected to the Arduino’s software-serial RX (D2), with a shared ground.
3. The Arduino running `firmware/ARDUINO_CODE/ARDUINO_CODE.ino`.
4. Motor driver and servos wired to the pins in the sketch and pinout document.

Use an appropriate external supply for motors and servos; do not assume the microcontroller board can provide their current. Keep the robot’s wheels raised during first power-on and action testing. Keep a reachable way to disconnect motor power.

### Wi-Fi secrets for ESP32

For the active bridge sketch, copy `firmware/esp32_cam_act_bridge/secrets.example.h` to `firmware/esp32_cam_act_bridge/secrets.h` and set `WIFI_SSID` and `WIFI_PASSWORD`. The `secrets.h` file is ignored by Git. Do not commit real network credentials.

The alternate `ESP32_main` sketch has its own matching `secrets.example.h` if you need to inspect or build that sketch. It is not needed for the documented action loop.

## Laptop setup

### Requirements

- Python 3 and a working microphone/input device.
- FFmpeg available on `PATH` for OpenAI Whisper.
- A Groq API key.
- For GPU inference, a working CUDA-enabled PyTorch installation; otherwise set `USE_GPU = False` in `laptop/config.py` or allow the code to fall back to CPU.
- The laptop and ESP32 on the same Wi-Fi/LAN. UDP broadcast and local HTTP traffic must be allowed by the network and host firewall.

Create a virtual environment from the repository root and install the Python packages:

```bash
python -m venv .venv
```

Activate it, then run:

```bash
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
```

Copy `.env.example` to `.env`, then add your Groq key. `.env` is ignored by Git:

```dotenv
GROQ_API_KEY=your_groq_api_key
GROQ_MODEL=openai/gpt-oss-20b
```

The default model is configurable because hosted model availability can change. Check the provider’s current model list if requests fail.

## Run

1. Configure and upload the active ESP32 action bridge sketch. Create its local `secrets.h` first.
2. Upload `firmware/ARDUINO_CODE/ARDUINO_CODE.ino` to the Arduino.
3. Connect the laptop and ESP32 to the same trusted local network.
4. From the repository root, activate the virtual environment and start the laptop loop:

```bash
cd laptop
python main.py
```

The laptop searches for the ESP32 for up to ten seconds. If found, speak in English near the microphone. The current loop listens in fixed 3.5-second chunks and uses Whisper’s `base` model by default. Use `Ctrl+C` to stop the laptop process. To stop physical motion, issue a recognized `STOP_ALL` action or disconnect motor power.

## Configuration

Edit `laptop/config.py` for sample rate, chunk duration, audio energy thresholds, Whisper model, GPU preference, UDP port, and HTTP action path. The HTTP path must match the ESP32 bridge (`/ACT`) and the UDP port must match both laptop and ESP32 (`4210`).

The language model’s allowed action set is in `laptop/brain_llm.py`. The Arduino firmware implements the action names it recognizes; unknown actions fall back to idle. Keep the prompt’s action list and the Arduino parser aligned when extending the robot.

## Known limitations

- This is experimental firmware. It has no safety-rated obstacle avoidance, authentication, encryption, or guaranteed motor-command timeout.
- The ESP32 `/ACT` HTTP endpoint is unauthenticated. Use only on a trusted private LAN; do not port-forward or expose it to the public internet.
- `firmware/ESP32_main/ESP32_main.ino` does not forward actions and is not interchangeable with the active bridge.
- The wiring workbook lists an ultrasonic sensor on Arduino D2/D3 and an I²C display on A4/A5. The supplied Arduino firmware does not implement either peripheral; D2 is already used as the ESP32 serial RX. Do not wire the ultrasonic trigger to D2 with this firmware. Resolve that pin conflict before adding the sensor.
- `STOP_ALL` maps to the Arduino idle state, which stops the motors. Firmware movement does not currently have an independent timeout; keep power access available during tests.
- The audio energy detector is a simple threshold heuristic. Noise, microphone gain, and room acoustics may require tuning.
- The model can return malformed or unsuitable content. The software validates the action name but is not a safety system.
- Groq receives the recognized speech text. Do not speak confidential information while using the hosted model.
- Model IDs and hosted API behavior can change; choose a currently supported model in `.env` if needed.

## Contributing

Bug reports and carefully scoped improvements are welcome. Please include the board models, wiring changes, operating system, Python version, and relevant serial/log output. Never include API keys, Wi-Fi credentials, private recordings, or personal data in issues or pull requests.

## License

Released under the MIT License. See [LICENSE](LICENSE). Third-party packages and hardware remain subject to their own terms.

## Acknowledgements

- OpenAI Whisper for local speech recognition.
- Groq for hosted language-model inference.
- Arduino, ESP32, and the maintainers of the Python libraries listed in `requirements.txt`.
