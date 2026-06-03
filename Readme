# Auditory Awareness Glasses

Real-time wearable assistive glasses for deaf and hard-of-hearing users. The system detects, classifies, and localizes environmental sounds, then alerts the wearer through NeoPixel LEDs (visual) and haptic vibration motors (tactile) built into the glasses frame.

## Team

- **Lukina Chen** — Hardware lead, TDOA signal processing, PCB design (rear boards + main board)
- **Anuj Kamasamudram** — ML & software lead, YAMNet sound classification, front board PCB design
- **Yilu Huang** — Signal & firmware lead, GPIO alert dispatch timing, main board v2 design

## System Architecture

```
4× T5838 PDM Mics ──→ TXS0108 Level Shifter ──→ XIAO ESP32-S3
    (1.8V)              (1.8V ↔ 3.3V)              (3.3V)
                                                       │
                                    ┌──────────────────┤
                                    ▼                  ▼
                              3× NeoPixel LED    2× ERM Motor
                              (direction)        (rear alert)
```

All processing runs on-device with no internet connection. The ESP32-S3 captures audio from four PDM microphones, runs a quantized YAMNet model for sound classification, and uses TDOA (Time Difference of Arrival) across mic pairs for direction estimation.

| Layer | Technology |
|-------|-----------|
| Sound classification | YAMNet (Google, 521 AudioSet classes), int8 quantized |
| Edge hardware | XIAO ESP32-S3 (8 MB flash + 8 MB PSRAM) |
| Microphones | 4× T5838 (MMICT5838-00-012) PDM MEMS, 1.8V |
| Visual output | 3× SK6812-MINI-E NeoPixel LEDs |
| Haptic output | 2× ERM coin vibration motors |
| Level shifting | TXS0108EPWR (TSSOP-20), 1.8V ↔ 3.3V |
| Power | LiPo battery → Xiao charger → XC6206P182MR-G 1.8V LDO |

## Sound → Alert Mapping

| Sound Category | LED Color | Motor |
|---------------|-----------|-------|
| Silence | OFF | Off |
| Speech / Voice | Blue | Off |
| Music | Purple | Off |
| Alarm / Siren | Red | Both vibrate |
| Animal sounds | Yellow | Off |
| Door / Knock | Orange | Off |
| Horn / Beeping | Orange | Rear vibrate |
| Vehicle | White | Rear vibrate |
| Phone ringing | Purple | Off |
| Other | Green | Off |

LED color encodes the sound category. Motor vibration triggers only for rear-origin sounds, alerting the user to threats they cannot see.

## Direction Estimation (TDOA)

Four microphones are arranged in two pairs to estimate direction:

- **Front pair** (PCB1): 2× T5838 on the glasses front frame — left/right discrimination
- **Rear pair** (PCB5 or PCB2/3): 2× T5838 near the ears — front/back discrimination

The ESP32-S3 runs two I2S PDM interfaces. Cross-correlation between mic pairs yields a time delay that maps to an angle of arrival. Results from the TDOA test rig (breadboard prototype):

| Direction | Accuracy |
|-----------|----------|
| Left / Right | 7/7 = 100% |
| Front | 3/3 = 100% |
| Back | 2/5 = 40% (breadboard contact issues) |

## Hardware: PCB Design

The system uses multiple PCBs connected by 0.5 mm pitch FPC cables.

### PCB1 — Front Board (Anuj)

Sits on the top of the glasses front frame (~60 mm long).

- 2× T5838 mic, 3× SK6812-MINI-E LED, XC6206 LDO, caps, WAKE combining diodes
- Connects to main board via 2× FH12 6-pin FPC

### PCB2 & PCB3 — Rear Boards (Lukina)

Sit at the ends of the glasses arms near the ears (~25 mm each). Same schematic, mirrored.

- 1× T5838 mic, 1× ERM motor, SI2302 MOSFET, 1N4148W diode, ferrite bead, 2× 100nF caps
- PCB2 mic SELECT → GND (left channel), PCB3 mic SELECT → 1V8 (right channel)
- Connects to main board via FH12 8-pin FPC
- Layout rule: mic at one end, motor at the other, separate GND pours with single-point join

### PCB4 — Main Board v1 (Lukina)

Central hub at the back of the head. Houses the MCU, level shifter, LDO, and all FPC connectors.

- U1: Xiao ESP32-S3 SMD, U2: XC6206 1.8V LDO, U3: TXS0108EPWR level shifter
- 4× FPC connectors (2× 6-pin for front board, 2× 8-pin for rear boards)
- JST PH 2-pin battery connector, PCM12SMTR SPDT switch
- Top side: Xiao + switch + battery connector. Bottom side: everything else.

### PCB5 — Main Board v2 (Yilu)

Combines PCB2, PCB3, and PCB4 into a single board. Rear mics and motor driver are integrated on-board. Connects to front board only via 2× 6-pin FPC.

Components: Xiao ESP32-S3 SMD, XC6206 LDO, TXS0108EPWR, 2× T5838 (MK3/MK4), SI2302 + 1N4148W motor driver, PCM12SMTR switch, JST PH battery connector.

## Firmware

### PlatformIO Configuration

```ini
[env:seeed_xiao_esp32s3]
platform = espressif32
board = seeed_xiao_esp32s3
framework = arduino
monitor_speed = 115200
build_flags = -DARDUINO_USB_CDC_ON_BOOT=1
```

### GPIO Pin Map (PCB5)

| GPIO | Xiao Pin | Function |
|------|----------|----------|
| GPIO1 | D0 | PDM_CLK (all mics clock) |
| GPIO2 | D1 | PDM_DATA (rear mics, through TXS0108) |
| GPIO3 | D2 | WAKE_JST_3V3 |
| GPIO4 | D3 | PDM_THSEL |
| GPIO5 | D4 | MTR_PWM (ERM motor via MOSFET) |
| GPIO6 | D5 | PDM_DATA_FRONT (front mics) |
| GPIO43 | D6 | LED_DATA (NeoPixel) |
| GPIO9 | Pin 11 | WAKE_M4_3V3 |

### I2S PDM Channel Mapping

ESP32-S3 I2S swaps left/right relative to the T5838 convention:

```cpp
// samples[i]   = Right channel (MK3, SELECT=1V8)
// samples[i+1] = Left channel  (MK4, SELECT=GND)
```

### Basic Mic Test Code

```cpp
#include <Arduino.h>
#include <driver/i2s.h>

#define I2S_WS   1   // PDM_CLK
#define I2S_DIN  2   // PDM_DATA
#define SAMPLE_RATE 16000
#define BUFFER_SIZE 512

int16_t samples[BUFFER_SIZE];

void setup() {
    Serial.begin(115200);
    delay(2000);

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = BUFFER_SIZE,
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_DIN,
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}

void loop() {
    size_t bytes_read;
    i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytes_read, portMAX_DELAY);

    int num = bytes_read / sizeof(int16_t);
    int16_t min_l = 32767, max_l = -32768;
    int16_t min_r = 32767, max_r = -32768;

    for (int i = 0; i < num; i += 2) {
        if (samples[i] > max_r) max_r = samples[i];
        if (samples[i] < min_r) min_r = samples[i];
        if (samples[i+1] > max_l) max_l = samples[i+1];
        if (samples[i+1] < min_l) min_l = samples[i+1];
    }

    Serial.printf("L[%d,%d] Range:%d | R[%d,%d] Range:%d\n",
                  min_l, max_l, max_l - min_l,
                  min_r, max_r, max_r - min_r);
    delay(200);
}
```

Expected signal levels:

| State | Range |
|-------|-------|
| Quiet room | 5,000 – 15,000 |
| Normal speech | 30,000 – 65,535 |
| Loud clap | 50,000 – 65,535 |
| Data line disconnected | Fixed at -30,935 |

## ML Pipeline (Laptop Prototyping)

The ML pipeline runs on a laptop for prototyping. In production, the quantized model runs directly on the ESP32-S3.

### Repository Layout

```
DetectionGlasses/
└── YamnetTest/
    ├── yamnet_mic_test.py            # Live mic classification (no hardware)
    ├── yamnet_esp32_feasibility.py   # TFLite conversion + ESP32 feasibility report
    ├── yamnet_serial.py              # Laptop → USB serial → XIAO NeoPixel bridge
    ├── yamnet_accuracy_test.py       # float32 vs int8 accuracy on ESC-50
    ├── yamnet_urbansound_test.py     # Accuracy on UrbanSound8K
    ├── neopixel_noise_test/
    │   └── neopixel_noise_test.ino   # Arduino NeoPixel LED control sketch
    └── tflite_models/
        ├── yamnet_full.tflite        # ~3.7 MB float32
        ├── yamnet_int8.tflite        # ~0.9 MB int8 quantized (ESP32 target)
        └── yamnet_saved_model/
```

### Quick Start

```bash
# Live mic demo (laptop only, no hardware)
pip install tensorflow tensorflow-hub sounddevice numpy scipy
python YamnetTest/yamnet_mic_test.py

# Generate TFLite models + ESP32 feasibility report
pip install tensorflow tensorflow-hub numpy
python YamnetTest/yamnet_esp32_feasibility.py

# Serial bridge (laptop mic → USB → XIAO NeoPixels)
pip install tensorflow tensorflow-hub sounddevice numpy pyserial
python YamnetTest/yamnet_serial.py --port /dev/cu.usbmodem1101

# Accuracy benchmarks
pip install tensorflow tensorflow-hub numpy scipy soundfile librosa tqdm
python YamnetTest/yamnet_accuracy_test.py --esc50-dir YamnetTest/downloads/ESC-50-master
python YamnetTest/yamnet_urbansound_test.py --limit 200
```

### Serial Protocol

```
C:speech:0.92:180    # category, confidence, brightness (0-255)
B:180                # brightness-only update
T:0.50               # set confidence threshold
OFF                  # turn LEDs off
```

### ESP32-S3 Model Feasibility

| Resource | Requirement | ESP32-S3 (N8R8) | Status |
|----------|------------|------------------|--------|
| Flash | ~0.9 MB (int8) | 8 MB | ✅ |
| RAM (activations) | ~300–400 KB | 8 MB PSRAM | ✅ |
| Inference time | ~200 ms target | TBD | — |

## Known Issues

- **TXS0108 OE wiring bug (PCB5):** R1 was routed to 3V3 instead of 1V8. OE max is VCCA+0.5V = 2.3V. Fixed with fly wire. Must be corrected in next revision.
- **MK3 weak signal (PCB5):** Right-channel mic produces very low Range (~100 vs expected ~10,000). Likely a soldering defect. Needs resolder or replacement.
- **FPC intermittent contact (PCB3):** Data line drops to fixed -30,935 value. Caused by loose FPC cable or connector lock. Resolved by reinserting and locking FPC.
- **ESP32 USB boot issue:** After uploading certain code, USB CDC may fail to initialize. Fix: double-click RESET to enter bootloader, then immediately upload.

## Bill of Materials

| Component | Part Number | Qty | Notes |
|-----------|------------|-----|-------|
| MCU | XIAO ESP32-S3 SMD | 1 | Castellated pads |
| Microphone | MMICT5838-00-012 (T5838) | 4 | PDM MEMS, 1.8V |
| Level shifter | TXS0108EPWR | 1 | TSSOP-20, 1.8V ↔ 3.3V |
| 1.8V LDO | XC6206P182MR-G | 1 | SOT-23 |
| NeoPixel LED | SK6812-MINI-E | 3 | RGB addressable |
| ERM motor | 10mm coin type | 2 | Vibration haptic |
| MOSFET | SI2302 | 1–2 | SOT-23, motor switch |
| Flyback diode | 1N4148W | 1–2 | SOD-323 |
| Ferrite bead | 600Ω 0402 | 1–2 | Motor power filtering |
| FPC connector (6-pin) | FH12-6S-0.5SH | 2–4 | Front board connection |
| FPC connector (8-pin) | FH12-8S-0.5SH | 2 | Rear board connection |
| Battery connector | JST PH 2-pin | 1 | LiPo battery |
| Switch | PCM12SMTR | 1 | SPDT SMD |
| Battery | LiPo 500–1000 mAh | 1 | 3.7V nominal |

## GitHub

[https://github.com/LukinaChen/TECHIN-515--Auditory-awareness-glasses](https://github.com/LukinaChen/TECHIN-515--Auditory-awareness-glasses)
