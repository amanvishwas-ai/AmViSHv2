# -------------------------------------------------
# tts_engine.py
# Stable TTS using pyttsx3 v2.91
# -------------------------------------------------

import pyttsx3
import threading

_engine = None
_engine_lock = threading.Lock()

def _init_engine():
    global _engine
    if _engine is None:
        print("[TTS] Initializing pyttsx3 engine (v2.91)")
        _engine = pyttsx3.init()
        _engine.setProperty("rate", 170)
        _engine.setProperty("volume", 1.0)

def speak(text: str):
    if not text:
        print("[TTS] Empty text, ignored")
        return

    with _engine_lock:
        _init_engine()
        try:
            print(f"[TTS] Speaking: {text}")
            _engine.say(text)
            _engine.runAndWait()
            print("[TTS] Done speaking")
        except Exception as e:
            print("[TTS] ERROR:", e)
