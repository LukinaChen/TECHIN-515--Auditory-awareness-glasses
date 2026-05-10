/*
 * 4-Mic TDOA + Motor + LED Alert System
 * Board: Xiao ESP32-S3
 * 
 * I2S0 (front pair):
 *   SCK → GPIO7 (D8), WS → GPIO8 (D9), SD → GPIO9 (D10)
 *   Mic1 L/R → GND (front left), Mic2 L/R → 3.3V (front right)
 *
 * I2S1 (rear pair):
 *   SCK → GPIO1 (D0), WS → GPIO2 (D1), SD → GPIO3 (D2)
 *   Mic3 L/R → GND (rear left), Mic4 L/R → 3.3V (rear right)
 *
 * Motor (via S8050 NPN + 1kΩ + 1N4148):
 *   Left motor  → GPIO4 (D3)
 *   Right motor → GPIO6 (D5)
 *
 * NeoPixel LED:
 *   DIN → GPIO5 (D4), VDD → 3.3V, GND → GND
 */

#include <Arduino.h>
#include <driver/i2s.h>
#include <Adafruit_NeoPixel.h>

// ----- I2S0 Pins (front pair) -----
#define I2S0_SCK  7
#define I2S0_WS   8
#define I2S0_SD   9

// ----- I2S1 Pins (rear pair) -----
#define I2S1_SCK  1
#define I2S1_WS   2
#define I2S1_SD   3

// ----- Motor Pins -----
#define MTR_LEFT  4   // GPIO4 (D3)
#define MTR_RIGHT 6   // GPIO6 (D5)

// ----- NeoPixel LED -----
#define LED_PIN   5   // GPIO5 (D4)
#define LED_COUNT 1
Adafruit_NeoPixel led(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ----- Audio Settings -----
#define SAMPLE_RATE     44100
#define BUFFER_LEN      512
#define CAPTURE_SAMPLES 1200
#define MAX_DELAY       20
#define CLAP_THRESHOLD  15000
#define COOLDOWN_MS     800
#define ALERT_DURATION  500  // motor vibrate + LED on for 1 second

// ----- Buffers -----
int32_t raw_i2s0[BUFFER_LEN];
int32_t raw_i2s1[BUFFER_LEN];

int32_t front_left[CAPTURE_SAMPLES];
int32_t front_right[CAPTURE_SAMPLES];
int32_t rear_left[CAPTURE_SAMPLES];
int32_t rear_right[CAPTURE_SAMPLES];

int capture_index = 0;
bool capturing = false;
bool ready_to_analyze = false;
int trial_number = 0;
unsigned long last_trigger_time = 0;
unsigned long alert_start_time = 0;
bool alerting = false;

void setup_i2s(i2s_port_t port, int sck, int ws, int sd) {
  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = BUFFER_LEN,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pins = {
    .bck_io_num   = sck,
    .ws_io_num    = ws,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = sd
  };

  i2s_driver_install(port, &config, 0, NULL);
  i2s_set_pin(port, &pins);
  i2s_zero_dma_buffer(port);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Setup motors
  pinMode(MTR_LEFT, OUTPUT);
  pinMode(MTR_RIGHT, OUTPUT);
  digitalWrite(MTR_LEFT, LOW);
  digitalWrite(MTR_RIGHT, LOW);

  // Setup NeoPixel
  led.begin();
  led.clear();
  led.show();

  // Setup I2S
  setup_i2s(I2S_NUM_0, I2S0_SCK, I2S0_WS, I2S0_SD);
  setup_i2s(I2S_NUM_1, I2S1_SCK, I2S1_WS, I2S1_SD);

  delay(500);

  Serial.println("==========================================");
  Serial.println("  4-Mic TDOA + Motor + LED Alert");
  Serial.println("  Left motor:  GPIO4 (D3)");
  Serial.println("  Right motor: GPIO6 (D5)");
  Serial.println("  NeoPixel:    GPIO5 (D4)");
  Serial.println("==========================================");
  Serial.println("Ready! Clap to test direction + alert.");
  Serial.println("------------------------------------------");
}

void remove_dc(int32_t* buf, int len) {
  int64_t sum = 0;
  for (int i = 0; i < len; i++) sum += buf[i];
  int32_t dc = sum / len;
  for (int i = 0; i < len; i++) buf[i] -= dc;
}

int compute_tdoa(int32_t* buf_a, int32_t* buf_b) {
  remove_dc(buf_a, CAPTURE_SAMPLES);
  remove_dc(buf_b, CAPTURE_SAMPLES);

  float max_corr = 0;
  int best_delay = 0;

  for (int d = -MAX_DELAY; d <= MAX_DELAY; d++) {
    float corr = 0;
    int count = 0;
    for (int i = 0; i < CAPTURE_SAMPLES; i++) {
      int j = i + d;
      if (j >= 0 && j < CAPTURE_SAMPLES) {
        corr += (float)buf_a[i] * (float)buf_b[j];
        count++;
      }
    }
    if (count > 0) corr /= count;
    if (corr > max_corr) {
      max_corr = corr;
      best_delay = d;
    }
  }
  return best_delay;
}

void trigger_alert(const char* lr_dir, const char* fr_dir) {
  // LED color based on left/right direction
  if (strcmp(lr_dir, "LEFT") == 0) {
    led.setPixelColor(0, led.Color(0, 0, 255));    // Blue = left
    Serial.println("  [LED] Blue (left)");
  } else if (strcmp(lr_dir, "RIGHT") == 0) {
    led.setPixelColor(0, led.Color(0, 255, 0));    // Green = right
    Serial.println("  [LED] Green (right)");
  } else {
    led.setPixelColor(0, led.Color(255, 255, 255)); // White = center
    Serial.println("  [LED] White (center)");
  }
  led.show();

  // Motor only vibrates for BACK sounds
  if (strcmp(fr_dir, "BACK") == 0) {
    if (strcmp(lr_dir, "LEFT") == 0) {
      digitalWrite(MTR_LEFT, HIGH);
      digitalWrite(MTR_RIGHT, LOW);
      Serial.println("  [MOTOR] Left vibrate (back-left)");
    } else if (strcmp(lr_dir, "RIGHT") == 0) {
      digitalWrite(MTR_LEFT, LOW);
      digitalWrite(MTR_RIGHT, HIGH);
      Serial.println("  [MOTOR] Right vibrate (back-right)");
    } else {
      digitalWrite(MTR_LEFT, HIGH);
      digitalWrite(MTR_RIGHT, HIGH);
      Serial.println("  [MOTOR] Both vibrate (back-center)");
    }
  } else {
    digitalWrite(MTR_LEFT, LOW);
    digitalWrite(MTR_RIGHT, LOW);
    Serial.println("  [MOTOR] Off (front/middle)");
  }

  alerting = true;
  alert_start_time = millis();
} 


void stop_alert() {
  digitalWrite(MTR_LEFT, LOW);
  digitalWrite(MTR_RIGHT, LOW);
  led.clear();
  led.show();
  alerting = false;
}

void analyze_direction() {
  // Left-Right: average of front pair AND rear pair
  int lr_delay_front = compute_tdoa(front_left, front_right);
  int lr_delay_rear = compute_tdoa(rear_left, rear_right);
  int lr_delay = (lr_delay_front + lr_delay_rear) / 2;

  // Front-Back: average of left side AND right side
  int fb_delay_left = compute_tdoa(front_left, rear_left);
  int fb_delay_right = compute_tdoa(front_right, rear_right);
  int fb_delay = (fb_delay_left + fb_delay_right) / 2;

  float lr_us = (float)lr_delay / SAMPLE_RATE * 1000000.0;
  float fb_us = (float)fb_delay / SAMPLE_RATE * 1000000.0;

  const char* lr_dir;
  if (lr_delay > 3) lr_dir = "RIGHT";
  else if (lr_delay < -3) lr_dir = "LEFT";
  else lr_dir = "CENTER";

  const char* fr_dir;
if (fb_delay > 5) fr_dir = "FRONT";
else if (fb_delay < -5) fr_dir = "BACK";
else fr_dir = "FRONT";  // default to FRONT (no motor) 

  trial_number++;

  Serial.println("==========================================");
  Serial.printf("  Trial #%d\n", trial_number);
  Serial.println("  ---- Left/Right ----");
  Serial.printf("  Front pair delay: %d, Rear pair delay: %d\n", lr_delay_front, lr_delay_rear);
  Serial.printf("  Average delay: %d samples (%.1f us)\n", lr_delay, lr_us);
  Serial.printf("  Direction: %s\n", lr_dir);
  Serial.println("  ---- Front/Back ----");
  Serial.printf("  Left side delay: %d, Right side delay: %d\n", fb_delay_left, fb_delay_right);
  Serial.printf("  Average delay: %d samples (%.1f us)\n", fb_delay, fb_us);
  Serial.printf("  Direction: %s\n", fr_dir);
  Serial.println("  ---- Combined ----");
  Serial.printf("  >>> %s - %s <<<\n", fr_dir, lr_dir);

  trigger_alert(lr_dir, fr_dir);

  Serial.println("==========================================");
}

void loop() {
  // Turn off alert after duration
  if (alerting && (millis() - alert_start_time > ALERT_DURATION)) {
    stop_alert();
    Serial.println("Ready for next clap...");
  }

  size_t bytes_read_0 = 0;
  size_t bytes_read_1 = 0;

  i2s_read(I2S_NUM_0, raw_i2s0, sizeof(raw_i2s0), &bytes_read_0, portMAX_DELAY);
  i2s_read(I2S_NUM_1, raw_i2s1, sizeof(raw_i2s1), &bytes_read_1, portMAX_DELAY);

  int samples_0 = bytes_read_0 / sizeof(int32_t);
  int samples_1 = bytes_read_1 / sizeof(int32_t);

  for (int i = 0; i < samples_0; i += 2) {
    // Front pair — swapped to match ESP32 I2S channel mapping
    int32_t fl = raw_i2s0[i + 1] >> 14;  // front left  (L/R → GND)
    int32_t fr = raw_i2s0[i] >> 14;      // front right (L/R → 3.3V)

    int32_t rl = 0, rr = 0;
    if (i < samples_1) {
      rl = raw_i2s1[i + 1] >> 14;        // rear left   (L/R → GND)
      rr = raw_i2s1[i] >> 14;            // rear right  (L/R → 3.3V)
    }

    // Check for loud sound to trigger capture
    if (!capturing && !ready_to_analyze && !alerting) {
      if ((abs(fl) > CLAP_THRESHOLD || abs(fr) > CLAP_THRESHOLD ||
           abs(rl) > CLAP_THRESHOLD || abs(rr) > CLAP_THRESHOLD) &&
          (millis() - last_trigger_time > COOLDOWN_MS)) {
        capturing = true;
        capture_index = 0;
        last_trigger_time = millis();
        Serial.println("CLAP detected! Capturing from all 4 mics...");
      }
    }

    // Capture samples from all 4 mics
    if (capturing) {
      if (capture_index < CAPTURE_SAMPLES) {
        front_left[capture_index] = fl;
        front_right[capture_index] = fr;
        rear_left[capture_index] = rl;
        rear_right[capture_index] = rr;
        capture_index++;
      } else {
        capturing = false;
        ready_to_analyze = true;
      }
    }
  }

  if (ready_to_analyze) {
    analyze_direction();
    ready_to_analyze = false;
  }
}