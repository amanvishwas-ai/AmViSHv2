import time
import requests

from audio_listener import record_chunk
from stt_whisper import transcribe
from brain_llm import llm_process
from tts_engine import speak
from reverse_discovery import discover_esp32
from config import ESP_ACT_ENDPOINT

print("[MAIN] Starting AmVish core loop")

# -------------------------------------------------
# ESP32 DISCOVERY
# -------------------------------------------------
ESP_IP = discover_esp32() 
if not ESP_IP:
     raise RuntimeError("[MAIN] ❌ ESP32 not found")

BASE_URL = f"http://{ESP_IP}"
print(f"[MAIN] ESP base URL = {BASE_URL}")

# -------------------------------------------------
# SEND ACT TO ESP
# -------------------------------------------------
def send_act_to_esp(act):
    print(f"[ESP] Sending ACT = {act}")
    try:
        r = requests.post(
            BASE_URL + ESP_ACT_ENDPOINT,
            data=f'{{"act":"{act}"}}',
            headers={"Content-Type": "application/json"},
            timeout=1
        )
        print(f"[ESP] Response code = {r.status_code}")
    except Exception as e:
        print(f"[ESP] ERROR sending ACT: {e}")

print("\n🤖 AmVish online. Speak now.\n")

# -------------------------------------------------
# MAIN LOOP
# -------------------------------------------------
while True:
    print("\n[MAIN] Listening cycle start")

    audio = record_chunk()
    if audio is None:
        time.sleep(0.1)
        continue

    text = transcribe(audio)
    if not text:
        print("[MAIN] No speech detected")
        time.sleep(0.1)
        continue

    print(f"[MAIN] User said: {text}")

    reply = llm_process(text)
    act = reply.get("ACT", "IDLE")
    say = reply.get("SAY", "null")

    print(f"[MAIN] LLM → ACT={act}, SAY={say}")

    send_act_to_esp(act)

    if say and str(say).lower() != "null":
        speak(say)
    else:
        print("[MAIN] Nothing to speak")

    time.sleep(0.3)
