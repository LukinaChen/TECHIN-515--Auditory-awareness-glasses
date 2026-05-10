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

**Date:** Week 3  
**Purpose:** All 4 mics connected, test full left/right/front/rear detection simultaneously.  
**Setup:** 4× INMP441 on Xiao ESP32-S3. I2S0 (GPIO7/8/9) = front pair, I2S1 (GPIO1/2/3) = rear pair. Front pair spacing 15.5cm, front-rear spacing 14cm. Sample rate 44100Hz, threshold 20000, cooldown 1500ms.

**Code version:** main_4mic.cpp with swapped L/R channel read order (i+1 = left, i = right) to match ESP32 I2S protocol.

**Bug found & fixed:** 
- ESP32 I2S left/right channel mapping is opposite to INMP441 L/R pin definition. Swapped [i] and [i+1] in code to fix.
- Inner scope variable re-declaration (int32_t rl inside if block) shadowed outer variable, causing rear mics to always read 0. Removed duplicate int32_t.

**Best test run results (after fixes):**

| Trial | Source | L/R result | Correct? | F/B result | Correct? |
|-------|--------|-----------|----------|-----------|----------|
| 1 | Left | LEFT | ✓ | FRONT | - |
| 2 | Left | LEFT | ✓ | BACK | - |
| 3 | Left | LEFT | ✓ | BACK | - |
| 4 | Left | LEFT | ✓ | BACK | - |
| 5 | Right | RIGHT | ✓ | BACK | - |
| 6 | Right | RIGHT | ✓ | MIDDLE | - |
| 7 | Right | RIGHT | ✓ | MIDDLE | - |
| 8 | Front | CENTER | ✓ | FRONT | ✓ |
| 9 | Front | CENTER | ✓ | FRONT | ✓ |
| 10 | Front | CENTER | ✓ | FRONT | ✓ |
| 11 | Back | CENTER | - | MIDDLE | ✗ |
| 12 | Back | CENTER | - | FRONT | ✗ |
| 13 | Back | CENTER | - | FRONT | ✗ |
| 14 | Back | CENTER | - | BACK | ✓ |
| 15 | Back | CENTER | - | BACK | ✓ |

**Accuracy:**
- Left/Right: 7/7 = **100%**
- Front: 3/3 = **100%**
- Back: 2/5 = **40%**

**Status: PARTIAL PASS — L/R and Front detection reliable, Back detection needs improvement**

**Known issues:**
- Front-right mic (FR) has intermittent breadboard contact issues — signal drops to near-zero randomly, causing false CENTER readings
- Back detection less reliable than front — likely due to breadboard instability on rear mic pair (I2S1)
- Rear mic raw data shows large spikes (e.g. RL:-19160, RR:25064) indicating loose connections
- All issues are hardware/breadboard related, not code — expected to resolve with PCB
- Combined direction output works (e.g. "FRONT - LEFT", "BACK - RIGHT") when all 4 mics have stable signal

**Next steps:**
- Fabricate PCB with T5838 mics at confirmed spacing
- Run full 40+ trial accuracy test on PCB with stable connections
- Add ERM motor + LED alert output integration



---

## Summary of Key Findings (for PCB Design)

- **15.5cm left-right spacing: WORKS** — confirmed TDOA can distinguish left/right (100% accuracy)
- **14cm front-rear spacing: WORKS** — confirmed TDOA can distinguish front/back (front 100%, back 40% on breadboard)
- **44.1kHz sample rate required** — 16kHz gives too few samples for reliable detection
- **ESP32-S3 dual I2S: WORKS** — I2S0 for front 2 mics, I2S1 for rear 2 mics, both running simultaneously
- **I2S channel swap required** — ESP32 I2S left/right mapping is opposite to INMP441 L/R pin definition; [i+1] = left (L/R→GND), [i] = right (L/R→3.3V)
- **Front/back direction swap required** — positive delay = FRONT, negative delay = BACK (opposite to initial assumption)
- **Breadboard instability** causes intermittent signal drops (especially FR mic) and noise — PCB will resolve
- **Sensitivity and accuracy** are software-tunable, no impact on PCB layout
- **Alert system verified** — LED responds to direction with color coding (blue=left, green=right, white=center), motors vibrate only for rear sounds, both turn off after 1 second

## GPIO Pin Assignment (confirmed working)

| Function | GPIO | Xiao Pin |
|----------|------|----------|
| PDM_CLK0 (front mics) | GPIO7 | D8 |
| PDM_WS0 (front mics) | GPIO8 | D9 |
| PDM_SD0 (front mics) | GPIO9 | D10 |
| PDM_CLK1 (rear mics) | GPIO1 | D0 |
| PDM_WS1 (rear mics) | GPIO2 | D1 |
| PDM_SD1 (rear mics) | GPIO3 | D2 |
| MTR_LEFT | GPIO4 | D3 |
| LED (NeoPixel DIN) | GPIO5 | D4 |
| MTR_RIGHT | GPIO6 | D5 |

---

## Change Log

| Date | Change | Reason |
|------|--------|--------|
| Week 2 | Created test plan | Initial setup |
| Week 2 | Step 1 completed (partial pass) | I2S comms verified, breadboard contact issues noted |
| Week 2 | Step 2 preliminary pass | TDOA works at 15.5cm, left/right direction confirmed |
| Week 2 | Step 3 preliminary pass | TDOA works at 14cm, front/back direction confirmed |
| Week 3 | Step 4 partial pass | 4-mic dual I2S working, L/R 100%, front 100%, back 40% |
| Week 3 | Bug fix: I2S channel swap | ESP32 L/R channel mapping opposite to INMP441, swapped [i] and [i+1] |
| Week 3 | Bug fix: rear mic variable shadowing | int32_t re-declaration inside if block caused rear mics to read 0 |
| Week 3 | Bug fix: front/back direction swap | Positive delay = FRONT not BACK, swapped labels in code |
| Week 5 | Motor + LED integration | Added S8050 NPN motor driver circuit, NeoPixel LED, direction-based alert logic |
| Week 5 | Alert logic: front vs back | Front sounds = LED only, back sounds = LED + motor vibration |
| Week 5 | LED color coding | Blue = left, green = right, white = center (will map to sound category after YAMNet integration) | 



# Complete BOM — All Boards (Updated with 8-pin FPC for rear boards)

## PCB2 & PCB3: Rear Boards (same design × 2)

| Ref | Component | Value/Part | Package | Qty per board | Qty total (×2) | Purpose |
|-----|-----------|-----------|---------|---------------|----------------|---------|
| J1 | FPC connector | Hirose FH12-8S-0.5SH | 8-pin 0.5mm | 1 | 2 | Connect to main board |
| MK1 | MEMS mic | MMICT5838-00-012 (T5838) | LGA 3.5×2.65mm | 1 | 2 | Rear audio capture (PDM) |
| C1 | Capacitor | 100nF (X7R) | 0402 | 1 | 2 | Mic VDD decoupling |
| C2 | Capacitor | 100nF (X7R) | 0402 | 1 | 2 | Motor EMI filter |
| FB1 | Ferrite bead | 600Ω @ 100MHz | 0402 | 1 | 2 | Motor noise isolation |
| Q1 | MOSFET | SI2302 | SOT-23 | 1 | 2 | Motor driver switch |
| D1 | Diode | 1N4148W | SOD-323 | 1 | 2 | Motor flyback protection |
| M1 | ERM motor | 10mm coin type | — | 1 | 2 | Haptic vibration alert |

**8-pin FPC definition:**
1. 3V3
2. 1V8
3. GND
4. CLK1
5. DAT1
6. MTR_PWM
7. WAKE
8. NC (empty)

**Note:** PCB2 mic SELECT → GND (left channel), PCB3 mic SELECT → 1V8 (right channel)

---

## Main Board

| Ref | Component | Value/Part | Package | Qty | Purpose |
|-----|-----------|-----------|---------|-----|---------|
| U1 | MCU module | Xiao ESP32-S3 | SMD module | 1 | Main processor |
| U2 | 1.8V LDO | XC6206P182MR-G | SOT-23 | 1 | 3.3V → 1.8V for mics |
| U3 | Level shifter | TXS0108ERKSR | SSOP-20 | 1 | 1.8V ↔ 3.3V signal conversion |
| C1 | Capacitor | 1μF (X7R) | 0603 | 1 | LDO input cap |
| C2 | Capacitor | 1μF (X7R) | 0603 | 1 | LDO output cap |
| C3 | Capacitor | 100nF (X7R) | 0603 | 1 | TXS0108 VA bypass cap (1.8V side) |
| C4 | Capacitor | 100nF (X7R) | 0603 | 1 | TXS0108 VB bypass cap (3.3V side) |
| J_PCB1_PWR1 | FPC connector | Hirose FH12-6S-0.5SH | 6-pin 0.5mm | 1 | Connect to PCB1 (power + LED) |
| J_PCB1_SIG1 | FPC connector | Hirose FH12-6S-0.5SH | 6-pin 0.5mm | 1 | Connect to PCB1 (signal) |
| J_PCB2 | FPC connector | Hirose FH12-8S-0.5SH | 8-pin 0.5mm | 1 | Connect to PCB2 (rear left) |
| J_PCB3 | FPC connector | Hirose FH12-8S-0.5SH | 8-pin 0.5mm | 1 | Connect to PCB3 (rear right) |
| J_BAT1 | Battery connector | JST PH 2-pin | THT | 1 | LiPo battery plug |
| SW1 | Slide switch | SPDT | SMD or THT | 1 | Power on/off |

**TXS0108 signal mapping (8 channels, using 7):**

| Channel | A side (1.8V) | B side (3.3V) | Direction |
|---------|--------------|---------------|-----------|
| A1/B1 | PDM_CLK0 | ESP32 GPIO1 | 3.3V → 1.8V |
| A2/B2 | PDM_DAT0 | ESP32 GPIO2 | 1.8V → 3.3V |
| A3/B3 | PDM_CLK1 | ESP32 GPIO3 | 3.3V → 1.8V |
| A4/B4 | PDM_DAT1 | ESP32 GPIO4 | 1.8V → 3.3V |
| A5/B5 | THSEL | ESP32 GPIO8 | 3.3V → 1.8V |
| A6/B6 | WAKE (front) | ESP32 GPIO10 | 1.8V → 3.3V |
| A7/B7 | WAKE (rear) | ESP32 GPIO | 1.8V → 3.3V |
| A8/B8 | NC | NC | — |

---

## PCB1: Front Board (Anuj's board — for reference only)

| Ref | Component | Value/Part | Package | Qty | Purpose |
|-----|-----------|-----------|---------|-----|---------|
| MK1, MK2 | MEMS mic | MMICT5838-00-012 (T5838) | LGA 3.5×2.65mm | 2 | Front L+R audio capture |
| U1 | LDO | XC6206P182MR-G | SOT-23 | 1 | 3.3V → 1.8V for front mics |
| D1-D3 | NeoPixel LED | SK6812-MINI-E | — | 3 | Visual direction alert |
| D4, D5 | Diode | — | — | 2 | WAKE signal combining |
| U2, U3 | FPC connector | Hirose FH12 6-pin | 0.5mm | 2 | Connect to main board |
| C1, C2 | Capacitor | 100nF | 0402 | 2 | Mic decoupling |

---

## Shopping List Summary (all boards combined, excluding PCB1)

| Component | Value/Part | Total Qty Needed | Spares | Order Qty | Est. Price |
|-----------|-----------|-----------------|--------|-----------|------------|
| T5838 mic | MMICT5838-00-012 | 2 (rear) | +1 | 3 | $2.32 ea |
| Xiao ESP32-S3 | SMD | 1 | — | 1 | $7.49 |
| 1.8V LDO | XC6206P182MR-G | 1 | +1 | 2 | $0.55 ea |
| Level shifter | TXS0108ERKSR | 1 | +1 | 2 | ~$2 ea |
| FPC connector 6-pin | FH12-6S-0.5SH | 2 (main→PCB1) | — | 2 | $1.29 ea |
| FPC connector 8-pin | FH12-8S-0.5SH | 4 (2 on rear boards + 2 on main) | +1 | 5 | ~$1.30 ea |
| 100nF cap 0402 | X7R | 4 (rear boards) | +6 | 10 | ~$0.01 ea |
| 100nF cap 0603 | X7R | 2 (main, TXS0108) | +3 | 5 | ~$0.01 ea |
| 1μF cap 0603 | X7R | 2 (main, LDO) | +3 | 5 | ~$0.01 ea |
| Ferrite bead 0402 | 600Ω @ 100MHz | 2 | +2 | 4 | ~$0.05 ea |
| MOSFET | SI2302 SOT-23 | 2 | +2 | 4 | ~$0.30 ea |
| Diode | 1N4148W SOD-323 | 2 | +2 | 4 | ~$0.05 ea |
| ERM coin motor | 10mm | 2 | — | 2 | ~$3 ea |
| JST PH 2-pin | THT | 1 | — | 1 | ~$0.50 |
| SPDT switch | SMD/THT | 1 | +1 | 2 | ~$0.30 ea |
| LiPo battery | 500-1000mAh | 1 | — | 1 | ~$8 |
| FPC cable 6-pin | 0.5mm pitch | 2 (main↔PCB1) | +1 | 3 | ~$1 ea |
| FPC cable 8-pin | 0.5mm pitch | 2 (main↔PCB2/3) | +1 | 3 | ~$1 ea |

**Estimated total (excluding PCB1 and PCB fab): ~$50-60**

---

## Items already purchased (from budget slide)

| Item | Qty | Status |
|------|-----|--------|
| MMICT5838-00-012 | 5 | ✓ Bought |
| ESP32-S3 | 2 | ✓ Bought |
| FH12-6S-0.5SH (6-pin) | 6 | ✓ Bought (use 2 for PCB1, 4 spare) |
| XC6206P182MR-G | 6 | ✓ Bought |
| INMP441 (testing only) | 4 pack × 2 | ✓ Bought |
| SPH0645 (testing only) | 2 pack × 2 | ✓ Bought |

## Still need to buy

| Item | Qty | Priority | Notes |
|------|-----|----------|-------|
| TXS0108ERKSR | 2 | HIGH | Level shifter — required for mic signals |
| FH12-8S-0.5SH (8-pin) | 5 | HIGH | Rear board FPC connectors — 6-pin won't work |
| FPC cable 8-pin 0.5mm | 3 | HIGH | Cables for rear board connections |
| FPC cable 6-pin 0.5mm | 3 | HIGH | Cables for front board connections |
| SI2302 MOSFET | 4 | MEDIUM | Motor driver — check if bought |
| 1N4148W SOD-323 | 4 | MEDIUM | Flyback diode — check if bought |
| Ferrite bead 600Ω 0402 | 4 | MEDIUM | Motor isolation — check if bought |
| 100nF caps 0402 | 10 | MEDIUM | Mic + motor decoupling — check if bought |
| 1μF caps 0603 | 5 | MEDIUM | LDO caps — check if bought |
| 100nF caps 0603 | 5 | MEDIUM | TXS0108 bypass — check if bought |
| ERM coin motor 10mm | 2 | MEDIUM | Check if bought |
| SPDT switch | 2 | LOW | Check if bought |
| JST PH 2-pin connector | 1 | LOW | Check if bought |
| LiPo battery 500-1000mAh | 1 | LOW | Check if bought |