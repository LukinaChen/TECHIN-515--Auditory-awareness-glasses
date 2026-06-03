# Project Handoff Summary — Auditory Awareness Glasses (Updated)

## Project Overview
Wearable assistive glasses for deaf/hard-of-hearing users. Detects, classifies, and localizes environmental sounds in real time. Alerts user through LED bar (visual) and haptic vibration motors (tactile) built into glasses frame.

---

## Team & Roles
- **Lukina Chen (you)** — Hardware lead, TDOA signal processing, PCB design for rear boards + main board
- **Anuj Kamasamudram** — ML & software lead, YAMNet sound classification, PCB1 (front board) design
- **Yilu Huang** — Signal & firmware lead, GPIO alert dispatch timing, designed PCB5 (main board v2)

---

## System Architecture
- **MCU:** Xiao ESP32-S3 (SMD version)
- **Mics:** 4× T5838 (MMICT5838-00-012) PDM MEMS microphones, 1.8V supply
- **Output:** 3× SK6812-MINI-E NeoPixel LEDs + 2× ERM coin motors
- **Power:** LiPo battery → Xiao (built-in charger + 3.3V LDO) → XC6206P182MR-G (1.8V LDO for mics)
- **Level shifter:** TXS0108EPWR (TSSOP-20) — 1.8V ↔ 3.3V conversion for all mic signals
- **No internet connection** — all processing on-device

---

## PCB Status Summary

### PCB2 & PCB3 (Rear Boards - Lukina's design)
- Schematic: ✅ Done
- PCB layout: ✅ Done
- DRC: ✅ Pass
- Gerber: ✅ Exported
- **PCB3 (Right) Testing:**
  - Mic: ✅ Confirmed working (tested with breakout board + ESP32-S3 on breadboard)
  - ERM motor: ✅ Confirmed working (controlled via MOSFET, GPIO must be pulled LOW to stop motor)
  - FPC connection: ⚠️ Intermittent contact issues (34601 fixed value when FPC disconnects)

### PCB4 (Main Board - Lukina's design)
- Schematic: ✅ Done
- PCB layout: ✅ Done
- DRC status:
  - 1 excluded error: B.Cu GND zone missing connection (ignorable — GND pads connected via F.Cu)
  - Remaining warnings: silkscreen clipping (ignorable)
  - Dangling track warnings: cleaned up (PDM_DAT1, MTR_PWM_R, 1V8, PDM_CLK1_1V8)
- Mounting holes: ✅ Added (4× MountingHole_2.1mm at corners)
- Board outline: ✅ Fixed (Edge.Cuts gap closed)
- **Not yet fabricated or assembled**

### PCB5 (Main Board v2 - Yilu's design, combines PCB2/3 + main)
- Schematic: ✅ Done
- PCB fabricated: ✅ Done
- Components soldered: ✅ Done
- **Testing results:**
  - Power: ✅ 3.3V and 1.8V confirmed correct
  - TXS0108 level shifter: ✅ Working (after OE fix)
  - MK4 mic (SELECT=GND, Left channel): ✅ Working — Range ~10000 quiet, 50000+ clap
  - MK3 mic (SELECT=1V8, Right channel): ❌ Very weak signal — Range ~100 quiet, ~5000 speaking
  - ERM motor: Not yet tested on PCB5
  - Front PCB connection: Not yet tested

### PCB1 (Front Board - Anuj's design)
- Not yet tested with main board

---

## Critical Bugs Found & Fixed

### 1. TXS0108 OE voltage error (PCB5)
- **Problem:** R1 (11kΩ) connected OE to 3V3 instead of 1V8
- **Impact:** OE max voltage is VCCA+0.5V = 2.3V, 3.3V exceeded limit
- **Fix:** Removed R1, flew wire from OE (Pin 10) to LDO VOUT (1.8V) with R1 in series
- **Status:** ✅ Fixed, OE now reads 1.8V

### 2. TXS0108 Pin 18 virtual solder (PCB5)
- **Problem:** Pin 18 (PDM_DATA B-side) had cold solder joint, ESP32 D1 read 0.2V instead of ~1.5V
- **Impact:** Neither MK3 nor MK4 data could reach ESP32
- **Fix:** Re-soldered Pin 18
- **Status:** ✅ Fixed, MK4 now works

### 3. MK3 weak signal (PCB5)
- **Problem:** MK3 Range only ~100 (should be ~10000), very weak response to sound
- **Likely cause:** MK3 soldering issue (VDD or DATA pad)
- **Status:** ❌ Unresolved — MK3 was removed for standalone testing, needs to be soldered back or replaced

---

## ESP32 I2S PDM Important Notes

### Channel swap
ESP32 I2S channels are swapped from T5838 convention:
- `samples[i]` = **Right channel** (MK3, SELECT=1V8)
- `samples[i+1]` = **Left channel** (MK4, SELECT=GND)

### Working I2S configuration (PCB5)
```cpp
#define I2S_WS   1   // PDM_CLK → GPIO1 (D0)
#define I2S_DIN  2   // PDM_DATA → GPIO2 (D1)

i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
};
```

### T5838 supply voltage
- Design voltage: 1.8V (for power saving and TXS0108 compatibility)
- Testing: Works at 3.3V (VDD range 1.6V–3.6V), but noise floor is higher
- PCB3 standalone testing used 3.3V on both VDD and 1V8 pins

### Signal levels (normal operation)
| State | Expected Range |
|-------|---------------|
| Quiet | 5000–15000 |
| Speaking | 30000–65535 |
| Clap | 50000–65535 |

### Common failure mode
- `-30935` fixed value = **data line disconnected** (FPC contact, virtual solder, or mic not present)

---

## PCB5 GPIO Pin Assignment (Yilu's design)

| GPIO | Xiao Pin | Function |
|------|----------|----------|
| GPIO1 | D0 | PDM_CLK (all mics clock) |
| GPIO2 | D1 | PDM_DATA (rear mics data, through TXS0108) |
| GPIO3 | D2 | WAKE_JST_3V3 |
| GPIO4 | D3 | PDM_THSEL |
| GPIO5 | D4 | MTR_PWM (ERM motor via MOSFET) |
| GPIO6 | D5 | PDM_DATA_FRONT (front mics data) |
| GPIO43 | D6 | LED_DATA (NeoPixel) |
| GPIO9 | Pin 11 | WAKE_M4_3V3 |

## TXS0108 Pin Map (PCB5)

| Physical Pin | Function | Side |
|-------------|----------|------|
| 1 | CLK net (A1) | 1.8V |
| 2 | VCCA (+1V8) | Power |
| 3 | DATA_BACK net (A2) | 1.8V |
| 4 | DATA_FRONT net (A3) | 1.8V |
| 5 | WAKE_JST_1V8 (A4) | 1.8V |
| 6 | THSEL net (A5) | 1.8V |
| 7 | WAKE_M4_1V8 (A6) | 1.8V |
| 8 | GND (A7) | — |
| 9 | GND (A8) | — |
| 10 | OE | Enable (→1V8 via R1) |
| 11 | GND | — |
| 12 | GND | — |
| 13 | GND (B7) | — |
| 14 | WAKE_M4_3V3 (B6) | 3.3V |
| 15 | PDM_THSEL (B5) | 3.3V |
| 16 | WAKE_JST_3V3 (B4) | 3.3V |
| 17 | PDM_DATA_FRONT (B3) | 3.3V |
| 18 | PDM_DATA (B2) | 3.3V |
| 19 | VCCB (Net-U1-3V3) | Power |
| 20 | PDM_CLK (B1) | 3.3V |

---

## PCB5 BOM

| Reference | Qty | Value | Footprint |
|-----------|-----|-------|-----------|
| Batteryconnector1 | 1 | S2B-PH-K-SLFSN | JST PH 2-pin |
| C1,C6,C7 | 3 | 2.2μF | 0603 |
| C2,C3,C4,C5,C8,C9 | 6 | 680nF | 0603 |
| D1 | 1 | Diode | SOD-123 |
| FB1 | 1 | Ferrite Bead | 0402 |
| J1_Right1 | 1 | FPC 6-pin (Right) | FH12-6S-0.5SH |
| J2_Left1 | 1 | FPC 6-pin (Left) | FH12-6S-0.5SH |
| J3 | 1 | ERM coin motor | Pin Header 2-pin |
| MK3, MK4 | 2 | MMICT5838-00-012 | T5838 |
| Q1 | 1 | SI2302 | SOT-23 |
| R1 | 1 | 11kΩ | 0603 |
| SW2 | 1 | PCM12SMTR | SPDT SMD |
| U1 | 1 | XIAO-ESP32-S3-SMD | Castellated SMD |
| U2 | 1 | XC6206P182MR-G | SOT-23 (1.8V LDO) |
| U3 | 1 | TXS0108EPWR | TSSOP-20 |

---

## FPC Connector Pin Definitions (PCB5)

### J2_Left1 (6-pin, to front board)
1. DATA_FRONT net
2. CLK net
3. THSEL net
4. WAKE_JST_1V8
5. NC
6. NC

### J1_Right1 (6-pin, to front board)
1. NC
2. +1V8
3. GND
4. LED_DATA
5. NC
6. NC

---

## What Needs to Be Done Next

1. ❌ Fix MK3 on PCB5 — resolder or replace with new T5838
2. ❌ Test ERM motor on PCB5 (use Serial command 'm'/'s')
3. ❌ Test front PCB (PCB1) connected to PCB5 via FPC
4. ❌ Test LED output
5. ❌ Full system integration test (all 4 mics + LED + motor)
6. ❌ Battery power test
7. ✅ PCB4 (Lukina's main board) — DRC clean, ready for Gerber export if needed as backup

---

## Items Already Purchased
- MMICT5838-00-012 × 5
- ESP32-S3 × 2
- FH12-6S-0.5SH (6-pin) × 6
- XC6206P182MR-G × 6
- INMP441 (testing) × 8
- SPH0645 (testing) × 4
- TXS0108EPWR × 2 (1 used on PCB5, 1 spare)
- FH12-8S-0.5SH (8-pin) × 5
- FPC cable 8-pin 0.5mm × 3
- FPC cable 6-pin 0.5mm × 3
- SI2302 MOSFET SOT-23 × 4
- 1N4148W SOD-323 × 4
- Ferrite bead 600Ω 0402 × 4
- Capacitors (0402 and 0603)
- ERM coin motor 10mm × 2
- SPDT switch × 2
- JST PH 2-pin × 1
- PCM12SMTR from DigiKey

## Still Need
- LiPo battery 500-1000mAh × 1
- Replacement T5838 for MK3 (if current one is damaged)
