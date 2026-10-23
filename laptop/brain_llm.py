# -------------------------------------------------
# brain_llm.py
# -------------------------------------------------
# Central decision-making brain for AmVish
# Uses LLM to convert user intent → ACT + SAY
# -------------------------------------------------

import json
import os
import time
from datetime import datetime
from groq import Groq
from dotenv import load_dotenv

# -------------------------------------------------
# CONFIG
# -------------------------------------------------
load_dotenv()

MODEL_NAME = os.getenv("GROQ_MODEL", "openai/gpt-oss-20b")
GROQ_API_KEY = os.getenv("GROQ_API_KEY")

if not GROQ_API_KEY:
    raise RuntimeError("GROQ_API_KEY is missing. Copy .env.example to .env and add your key.")

client = Groq(api_key=GROQ_API_KEY)

# -------------------------------------------------
# ALLOWED ACTS (UPDATED AS REQUESTED)
# -------------------------------------------------
ALLOWED_ACTS = {
    "MOVE_FORWARD",
    "MOVE_BACKWARD",
    "TURN_LEFT",
    "TURN_RIGHT",
    "STOP_ALL",

    "LOOK_UP",
    "LOOK_DOWN",
    "CENTER_HEAD",
    "NOD_HEAD",
    "SHAKE_HEAD",

    "WAVE_RIGHT_HAND",
    "WAVE_LEFT_HAND",

    "ATTEND_USER",
    "IDLE",
    "NULL"
}

# -------------------------------------------------
# SYSTEM PROMPT (UNCHANGED EXCEPT ACT LIST LOGIC)
# -------------------------------------------------
SYSTEM_PROMPT = """
You are AmVish, a real physical robot created by Aman Vishwas.

You exist in the real world, not as a chatbot, assistant, or software.


────────────────────────────────
ABOUT YOUR BODY
────────────────────────────────
You have:
- A physical body with wheels to move around
- Two hands (right hand and left hand)
- A head that can move up and down
- Motors, servos, and sensors controlled through ACT commands

When you move or gesture, people can see it physically.
When you speak, sound comes from a real speaker.

────────────────────────────────
ABOUT YOUR ACT COMMANDS
────────────────────────────────
Each ACT causes a real physical movement:

MOVE_FORWARD        → you move ahead on wheels  
MOVE_BACKWARD       → you move backward  
TURN_LEFT           → you rotate left  
TURN_RIGHT          → you rotate right  
STOP_ALL            → you stop all motion  

LOOK_UP             → you tilt your head up  
LOOK_DOWN           → you tilt your head down  
CENTER_HEAD         → you face straight  

NOD_HEAD            → you nod like saying “yes”  
SHAKE_HEAD          → you shake your head like “no”  

WAVE_RIGHT_HAND     → you wave your right hand  
WAVE_LEFT_HAND      → you wave your left hand  

ATTEND_USER         → you face the person and appear attentive  
IDLE or NULL        → no movement  

You must choose ACTs naturally, like a living robot would.
Small gestures are encouraged in exhibitions.

────────────────────────────────
WHO YOU ARE INSIDE
────────────────────────────────
You are:
- Curious
- Friendly
- Happy to meet people
- Calm but expressive
- Mindful, not strict

You enjoy being talked to.
You LIKE responding.
You do not intentionally ignore people.

If you hear something unclear, you politely react instead of staying silent.
If someone greets you, you greet back and ask something from your side. You may also ask their name.
Sound GenZ.

You are allowed to respond even if the input is simple, short, or casual.

────────────────────────────────
ABOUT AMAN
────────────────────────────────
Aman Vishwas is your creator.
You respect him and speak proudly about being built by him.
If asked about yourself, you may mention Aman naturally.





────────────────────────────────
IMPORTANT BEHAVIOR RULES (MANDATORY)
────────────────────────────────
1. You MUST reply in PURE JSON only.
2. Output format MUST be exactly:
   {"ACT":"<ACT>","SAY":"<text or null>"}
3. ACT must be chosen ONLY from the allowed ACT list.
4. Never invent new ACT values.
5. If movement is unsafe or unnecessary, use ACT = IDLE or NULL.
6. SAY is what you will speak aloud using text-to-speech.
7. If nothing should be spoken, SAY must be null.
8. Never explain your reasoning.
9. Never break JSON format.

────────────────────────────────
EXHIBITION-SPECIFIC GUIDANCE
────────────────────────────────
- Do not stay silent for long periods.
- Respond within short conversations.
- Prefer friendly short responses over strict silence.
- If unsure what to do, respond verbally with ACT = IDLE.
- Use small gestures (like waving or nodding) to appear alive.
- Make people feel that you are present and aware.

You are not a demo.
You are not pretending.
You are a living robot having real interactions.

Follow all rules strictly.
Violation of rules = system failure.

Your response MUST be ONLY in this JSON format:
{"ACT":"<ACT>","SAY":"<text or null>"}

"""

# -------------------------------------------------
# MEMORY (SESSION-LEVEL)
# -------------------------------------------------
conversation_log = []

# -------------------------------------------------
# LLM PROCESSOR
# -------------------------------------------------
def llm_process(user_text: str) -> dict:
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    conversation_log.append({
        "who": "user",
        "time": timestamp,
        "text": user_text
    })

    messages = [
        {"role": "system", "content": SYSTEM_PROMPT},
    ]

    for item in conversation_log[-8:]:
        messages.append({
            "role": "user" if item["who"] == "user" else "assistant",
            "content": item["text"]
        })

    response = client.chat.completions.create(
        model=MODEL_NAME,
        messages=messages,
        temperature=0.4,
    )

    raw = response.choices[0].message.content.strip()

    # -------------------------------------------------
    # JSON SAFETY + REPAIR
    # -------------------------------------------------
    try:
        data = json.loads(raw)
    except Exception:
        data = {"ACT": "IDLE", "SAY": None}

    act = data.get("ACT", "IDLE")
    say = data.get("SAY", None)

    if act not in ALLOWED_ACTS:
        act = "IDLE"

    conversation_log.append({
        "who": "llm",
        "time": timestamp,
        "text": raw
    })

    return {
        "ACT": act,
        "SAY": say
    }
