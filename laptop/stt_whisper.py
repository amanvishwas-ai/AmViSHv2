import whisper
import torch
import numpy as np
from config import *

device = "cuda" if (USE_GPU and torch.cuda.is_available()) else "cpu"
print(f"[STT] Loading Whisper model on {device}")
model = whisper.load_model(WHISPER_MODEL, device=device)
print("[STT] Whisper loaded")

def transcribe(audio_np):
    if audio_np is None:
        print("[STT] No audio received")
        return ""

    audio_np = np.asarray(audio_np, dtype=np.float32).flatten()
    audio_np = np.clip(audio_np, -1.0, 1.0)

    with torch.no_grad():
        result = model.transcribe(
            audio_np,
            language="en",          # 🔒 force English
            fp16=(device == "cuda"),
            verbose=False
        )

    text = result.get("text", "").strip()
    print(f"[STT] Raw transcription: '{text}'")

    # ---- Garbage / hallucination filter ----
    if len(text) < 2:
        print("[STT] Ignored: too short")
        return ""

    if not any(c.isalpha() for c in text):
        print("[STT] Ignored: no letters")
        return ""

    return text
