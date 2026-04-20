/*
 * Step 2: TDOA Left/Right Direction Test (v2)
 * Fixed: higher threshold, longer cooldown, better noise handling
 */

#include <Arduino.h>
#include <driver/i2s.h>

// ----- Pin Definitions -----
#define I2S_SCK   7
#define I2S_WS    8
#define I2S_SD    9

// ----- Audio Settings -----
#define SAMPLE_RATE   44100
#define I2S_PORT      I2S_NUM_0
#define BUFFER_LEN    512

// ----- TDOA Settings -----
#define CAPTURE_SAMPLES  2048
#define MAX_DELAY        20          // tighter: 15.5cm max is ~20 samples
#define CLAP_THRESHOLD   30000       // raised: only triggers on loud claps
#define COOLDOWN_MS      3000        // 3 seconds between detections
#define MIC_DISTANCE_CM  15.5
#define SOUND_SPEED_CM   34300.0

// ----- Buffers -----
int32_t raw_samples[BUFFER_LEN];
int32_t left_buf[CAPTURE_SAMPLES];
int32_t right_buf[CAPTURE_SAMPLES];

int capture_index = 0;
bool capturing = false;
bool ready_to_analyze = false;
int trial_number = 0;
unsigned long last_trigger_time = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("==========================================");
  Serial.println("  TDOA Left/Right Direction Test v2");
  Serial.println("  Mic spacing: 15.5cm");
  Serial.println("  Threshold: 30000 (loud clap only)");
  Serial.println("  Cooldown: 3 seconds");
  Serial.println("==========================================");

  i2s_config_t i2s_config = {
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

  i2s_pin_config_t pin_config = {
    .bck_io_num   = I2S_SCK,
    .ws_io_num    = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_zero_dma_buffer(I2S_PORT);
  delay(500);

  Serial.println("Ready! Clap LOUDLY to test direction.");
  Serial.println("------------------------------------------");
}

// Remove DC offset before correlation
void remove_dc(int32_t* buf, int len) {
  int64_t sum = 0;
  for (int i = 0; i < len; i++) {
    sum += buf[i];
  }
  int32_t dc = sum / len;
  for (int i = 0; i < len; i++) {
    buf[i] -= dc;
  }
}

int compute_tdoa() {
  // Remove DC offset from both channels
  remove_dc(left_buf, CAPTURE_SAMPLES);
  remove_dc(right_buf, CAPTURE_SAMPLES);

  float max_corr = 0;
  int best_delay = 0;

  for (int d = -MAX_DELAY; d <= MAX_DELAY; d++) {
    float corr = 0;
    int count = 0;

    for (int i = 0; i < CAPTURE_SAMPLES; i++) {
      int j = i + d;
      if (j >= 0 && j < CAPTURE_SAMPLES) {
        corr += (float)left_buf[i] * (float)right_buf[j];
        count++;
      }
    }

    if (count > 0) {
      corr /= count;
    }

    if (corr > max_corr) {
      max_corr = corr;
      best_delay = d;
    }
  }

  return best_delay;
}

void analyze_direction() {
  int delay_samples = compute_tdoa();
  
  float delay_us = (float)delay_samples / SAMPLE_RATE * 1000000.0;

  const char* direction;
  if (delay_samples > 3) {
      direction = ">>> BACK";
  } else if (delay_samples < -3) {
      direction = "<<< FRONT";
  } else {
      direction = "=== CENTER";
  }

  trial_number++;
  
  Serial.println("==========================================");
  Serial.printf("  Trial #%d\n", trial_number);
  Serial.printf("  Delay: %d samples (%.1f us)\n", delay_samples, delay_us);
  Serial.printf("  Direction: %s\n", direction);
  Serial.println("==========================================");
  Serial.println("Ready for next clap...");
}

void loop() {
  size_t bytes_read = 0;

  i2s_read(I2S_PORT, raw_samples, sizeof(raw_samples), &bytes_read, portMAX_DELAY);
  int samples_read = bytes_read / sizeof(int32_t);

  for (int i = 0; i < samples_read; i += 2) {
    int32_t left  = raw_samples[i] >> 14;
    int32_t right = raw_samples[i + 1] >> 14;

    // Check for loud sound, with cooldown
    if (!capturing && !ready_to_analyze) {
      if ((abs(left) > CLAP_THRESHOLD || abs(right) > CLAP_THRESHOLD) 
          && (millis() - last_trigger_time > COOLDOWN_MS)) {
        capturing = true;
        capture_index = 0;
        last_trigger_time = millis();
        Serial.println("CLAP detected! Capturing...");
      }
    }

    if (capturing) {
      if (capture_index < CAPTURE_SAMPLES) {
        left_buf[capture_index] = left;
        right_buf[capture_index] = right;
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