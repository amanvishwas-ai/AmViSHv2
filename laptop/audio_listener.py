import sounddevice as sd
import numpy as np
from config import *

ambient_energy_avg = None

def compute_energy(audio):
    return float(np.sqrt(np.mean(audio ** 2)))

def record_chunk():
    global ambient_energy_avg

    print("[AUDIO] Recording chunk...")
    audio = sd.rec(
        int(CHUNK_DURATION * SAMPLE_RATE),
        samplerate=SAMPLE_RATE,
        channels=1,
        dtype="float32"
    )
    sd.wait()
    audio = audio.squeeze()

    energy = compute_energy(audio)
    print(f"[AUDIO] Energy = {energy:.6f}")

    if ambient_energy_avg is None:
        ambient_energy_avg = energy
        sd.stop()
        return None

    if energy < ambient_energy_avg * 0.5:
        ambient_energy_avg = (1 - ALPHA) * ambient_energy_avg + ALPHA * energy
        sd.stop()
        return None

    if energy > ambient_energy_avg * MARGIN:
        print("[AUDIO] Speech detected!")
        sd.stop()
        return audio

    sd.stop()
    return None
