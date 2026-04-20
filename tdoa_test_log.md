# Auditory Awareness Glasses — TDOA Test Log

**Owner:** Lukina Chen (Hardware Lead)  
**Board:** Xiao ESP32-S3  
**Microphones:** INMP441 × 4  
**Start Date:** Week 2

---

## Hardware Specs

- Glasses frame width (left-right mic distance): **15.5 cm**
- Glasses arm length (front-rear mic distance): **14.0 cm**
- Max TDOA left-right: 15.5cm ÷ 343m/s = **452 μs ≈ 20 samples @44.1kHz**
- Max TDOA front-rear: 14.0cm ÷ 343m/s = **408 μs ≈ 18 samples @44.1kHz**
- Sampling rate: **44100 Hz**

---

## Step 1: I2S Communication Test

**Date:** Week 2  
**Purpose:** Verify that 2 INMP441 mics can be read simultaneously via I2S on the Xiao ESP32-S3.  
**Setup:** 2 mics on breadboard, no fixed spacing. L/R pin: Mic1→GND (left), Mic2→3.3V (right).

**Wiring:**

| INMP441 Pin | ESP32-S3 Pin |
|-------------|-------------|
| VDD         | 3.3V        |
| GND         | GND         |
| SCK         | GPIO7 (D8)  |
| WS          | GPIO8 (D9)  |
| SD          | GPIO9 (D10) |
| L/R (Mic1)  | GND         |
| L/R (Mic2)  | 3.3V        |

**Pass Criteria:**
- [x] Serial Monitor shows changing values on both left and right channels
- [x] Values respond to sound (clap, snap, speak)
- [ ] No constant zeros or overflow on either channel

**Results:**

| Test | Left channel | Right channel | Notes |
|------|-------------|---------------|-------|
| Silence | ~1000 | ~0-20 | Both channels active |
| Speak near Mic2 | 5000-6800 | 1000-2400 | Both respond, left higher than right |
| Speak near Mic1 | 100000+ | ~0 | Mic1 has large DC offset, likely breadboard contact issue |

**Status: PARTIAL PASS**

**Issues / Notes:**
- Mic1 shows large DC offset (~100000) when speaking close to it — likely breadboard contact issue, not hardware fault
- Mic2 values consistently lower than Mic1 — may be breadboard connection or individual mic variance
- Both mics confirmed working and responsive to sound
- Breadboard contact issues expected to resolve with PCB

---

## Step 2: Left-Right TDOA Test

**Date:** Week 2  
**Purpose:** Verify that 15.5cm mic spacing can distinguish left, center, and right sound sources using cross-correlation.  
**Setup:** 2 mics fixed at 15.5cm apart (glasses width). Sound source: hand clap at ~1m distance.

**Code version:** main_tdoa_v2.cpp (threshold=30000, cooldown=3s, DC offset removal)

**Pass Criteria:**
- [x] Cross-correlation shows positive delay for sound from right
- [x] Cross-correlation shows negative delay for sound from left
- [ ] Cross-correlation shows ~zero delay for sound from center (not yet tested systematically)
- [ ] Accuracy ≥ 80% across 20+ trials (not yet completed — enough for PCB design validation)

**Results:**

| Trial | Source direction | Computed delay (samples) | Delay (μs) | Detected direction | Correct? |
|-------|----------------|------------------------|-----------|-------------------|----------|
| 8 | Right | 7 | 158.7 | >>> RIGHT | Yes |
| 9 | Left | -20 | -453.5 | <<< LEFT | Yes |

**Status: PRELIMINARY PASS — TDOA works at 15.5cm spacing**

**Issues / Notes:**
- Delay of -20 samples (453.5μs) close to theoretical max (452μs) — at the physical limit
- v1 code (threshold=5000) was too sensitive, triggered on ambient noise, 230+ false trials
- v2 code (threshold=30000, 3s cooldown, DC removal) much more stable
- Reaction speed is slow but acceptable — will be tuned in software later
- Breadboard connection instability adds noise — PCB will improve accuracy
- Direction detection accuracy and speed are software-tunable, do not affect PCB design
- Full 40+ trial validation deferred to after PCB prototype

---

## Step 3: Front-Rear TDOA Test

**Date:** Week 2  
**Purpose:** Verify that 14cm mic spacing (glasses arm length) can distinguish front vs rear sound sources.  
**Setup:** Mic1 (L/R→GND) at front, Mic2 (L/R→3.3V) at rear, 14cm apart. Same code as Step 2 with direction labels changed to FRONT/BACK.

**Pass Criteria:**
- [x] Cross-correlation shows negative delay for sound from front
- [x] Cross-correlation shows positive delay for sound from rear
- [ ] Accuracy ≥ 80% across 20+ trials (not yet completed — enough for PCB design validation)

**Results:**

| Trial | Source direction | Computed delay (samples) | Delay (μs) | Detected direction | Correct? |
|-------|----------------|------------------------|-----------|-------------------|----------|
| 10 | Front | -18 | -408.2 | <<< FRONT | Yes |
| 11 | Front | -10 | -226.8 | <<< FRONT | Yes |
| 12 | Back | 20 | 453.5 | >>> BACK | Yes |
| 13 | Back | 13 | 294.8 | >>> BACK | Yes |

**Status: PRELIMINARY PASS — TDOA works at 14cm spacing**

**Issues / Notes:**
- All 4 trials correctly detected front vs back direction
- Requires loud clap to trigger (threshold=30000) — sensitivity can be tuned in software
- 14cm spacing provides sufficient time difference for front/rear discrimination
- Full 40+ trial validation deferred to after PCB prototype

---

## Step 4: 4-Mic Full Direction Test

**Date:** ___________  
**Purpose:** All 4 mics connected, test full left/right/front/rear detection.  
**Setup:** 4 mics at final glasses positions. Uses ESP32-S3 dual I2S: I2S0 (front L+R), I2S1 (rear L+R).

**Pass Criteria:**
- [ ] All 4 mics read data simultaneously
- [ ] Can distinguish left, center, right, front, rear
- [ ] Accuracy ≥ 80% across 40+ trials per direction

**Results:**

| Trial | Source direction | Detected direction | Correct? | Notes |
|-------|----------------|--------------------|---------|----|
| | | | | |

**Accuracy:** _____ / _____ = _____%

**Issues / Notes:**



---

## Summary of Key Findings (for PCB Design)

- **15.5cm left-right spacing: WORKS** — confirmed TDOA can distinguish left/right
- **14cm front-rear spacing: WORKS** — confirmed TDOA can distinguish front/back
- **44.1kHz sample rate required** — 16kHz gives too few samples for reliable detection
- **ESP32-S3 dual I2S plan:** I2S0 for front 2 mics, I2S1 for rear 2 mics
- **Breadboard instability** causes DC offset and noise — PCB will resolve
- **Sensitivity and accuracy** are software-tunable, no impact on PCB layout

---

## Change Log

| Date | Change | Reason |
|------|--------|--------|
| Week 2 | Created test plan | Initial setup |
| Week 2 | Step 1 completed (partial pass) | I2S comms verified, breadboard contact issues noted |
| Week 2 | Step 2 preliminary pass | TDOA works at 15.5cm, left/right direction confirmed |
| Week 2 | Step 3 preliminary pass | TDOA works at 14cm, front/back direction confirmed |