/*
 * Environmental Light Intensity Dependent Street Lamp
 * Analog Circuits (UES301) minor project, TIET Patiala
 *
 * An LDR voltage divider is read on the ADC pin and the lamp (three LEDs driven
 * together) is dimmed with PWM: dark surroundings -> bright, daylight -> off.
 * The light reading is smoothed and the brightness fades gradually, so the lamp
 * never jumps suddenly.
 *
 * Wiring (read from the circuit diagram in the report, NodeMCU board):
 *   3V3 --- LDR --- A0 --- resistor --- GND       (more light -> higher A0 voltage)
 *   D6 (GPIO12) --> yellow LED --> resistor --> GND
 *   D5 (GPIO14) --> red LED    --> resistor --> GND
 *   D2 (GPIO4)  --> green LED  --> resistor --> GND
 *   Power: USB
 *
 * Boards:
 *   ESP8266 (NodeMCU)  - pins above match the diagram. Board: "NodeMCU 1.0 (ESP-12E)".
 *   ESP32              - the pins in the ESP32 block below are suggestions, NOT from
 *                        the diagram; wire them up (or change them) to match your board.
 *
 * Serial Monitor (115200 baud) prints CSV: ms,light,target,brightness,duty
 * Use it to calibrate NIGHT_LEVEL and DAY_LEVEL and to log real test data.
 */

#include <Arduino.h>

// ======================= Board pin map =======================
#if defined(ESP8266)
constexpr uint8_t LED_PINS[] = {12, 14, 4};   // D6 yellow, D5 red, D2 green
constexpr uint8_t LDR_PIN    = A0;
constexpr float   ADC_MAX    = 1023.0f;       // 10-bit ADC
#elif defined(ESP32)
constexpr uint8_t LED_PINS[] = {25, 26, 27};  // suggested pins
constexpr uint8_t LDR_PIN    = 34;            // ADC1 pin (input only); avoid ADC2 pins
constexpr float   ADC_MAX    = 4095.0f;       // 12-bit ADC
#else
#error "This sketch supports ESP8266 (NodeMCU) and ESP32 boards only."
#endif
constexpr size_t NUM_LEDS = sizeof(LED_PINS) / sizeof(LED_PINS[0]);

// ======================= Tunable settings =======================
// Light level is normalised 0.0 (dark) .. 1.0 (very bright).
constexpr float NIGHT_LEVEL = 0.10f;   // at or below this: lamp at full brightness
constexpr float DAY_LEVEL   = 0.50f;   // at or above this: lamp off
                                       // (starting values - calibrate from the serial log)
constexpr float MAX_BRIGHTNESS = 1.0f; // cap on lamp brightness, 0.0 .. 1.0
constexpr bool  INVERT_SENSOR  = false;// set true if the LDR is on the GND side of the divider

constexpr uint32_t FADE_TIME_MS = 3000;  // time for a full 0 -> 100 % change
constexpr float    SMOOTHING    = 0.05f; // sensor filter weight per tick (smaller = slower)
constexpr float    GAMMA        = 2.2f;  // perceived-brightness correction for the LEDs

constexpr uint8_t  OVERSAMPLE = 16;      // ADC readings averaged per tick
constexpr uint32_t TICK_MS    = 20;      // control loop period
constexpr uint32_t LOG_MS     = 500;     // serial log period

constexpr uint32_t PWM_FREQ_HZ = 1000;
constexpr uint8_t  PWM_BITS    = 10;
constexpr uint32_t PWM_MAX     = (1u << PWM_BITS) - 1;   // 1023

// ======================= State =======================
float    smoothedLight = 0.0f;
float    brightness    = 0.0f;   // current lamp brightness, 0.0 .. 1.0
uint32_t lastTick      = 0;
uint32_t lastLog       = 0;
uint32_t lastDuty      = 0xFFFFFFFF;

// ======================= PWM helpers =======================
static void pwmBegin() {
#if defined(ESP8266)
  analogWriteRange(PWM_MAX);
  analogWriteFreq(PWM_FREQ_HZ);
#endif
  for (size_t i = 0; i < NUM_LEDS; i++) {
#if defined(ESP8266)
    pinMode(LED_PINS[i], OUTPUT);
    analogWrite(LED_PINS[i], 0);
#elif defined(ESP32) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(LED_PINS[i], PWM_FREQ_HZ, PWM_BITS);
    ledcWrite(LED_PINS[i], 0);
#else  // ESP32, Arduino core 2.x
    ledcSetup(i, PWM_FREQ_HZ, PWM_BITS);
    ledcAttachPin(LED_PINS[i], i);
    ledcWrite(i, 0);
#endif
  }
}

static void pwmWriteAll(uint32_t duty) {
  if (duty == lastDuty) return;          // only touch the PWM when the value changes
  lastDuty = duty;
  for (size_t i = 0; i < NUM_LEDS; i++) {
#if defined(ESP8266)
    analogWrite(LED_PINS[i], duty);
#elif defined(ESP32) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(LED_PINS[i], duty);
#else
    ledcWrite(i, duty);
#endif
  }
}

// ======================= Sensing and control =======================
static float readLight() {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < OVERSAMPLE; i++) sum += analogRead(LDR_PIN);
  float level = ((float)sum / OVERSAMPLE) / ADC_MAX;
  if (level > 1.0f) level = 1.0f;
  return INVERT_SENSOR ? 1.0f - level : level;
}

// Darker surroundings -> higher target brightness (linear between the two levels).
static float targetBrightness(float light) {
  float t = (DAY_LEVEL - light) / (DAY_LEVEL - NIGHT_LEVEL);
  t = fminf(fmaxf(t, 0.0f), 1.0f);
  return t * MAX_BRIGHTNESS;
}

static uint32_t brightnessToDuty(float b) {
  return (uint32_t)(powf(b, GAMMA) * PWM_MAX + 0.5f);
}

// ======================= Arduino entry points =======================
void setup() {
  Serial.begin(115200);
  pwmBegin();

  smoothedLight = readLight();
  lastTick = lastLog = millis();

  Serial.println();
  Serial.println("# Environmental light dependent street lamp");
  Serial.println("ms,light,target,brightness,duty");
}

void loop() {
  const uint32_t now = millis();
  if (now - lastTick < TICK_MS) return;
  lastTick = now;

  // 1. Read and smooth the ambient light.
  smoothedLight += SMOOTHING * (readLight() - smoothedLight);

  // 2. Decide how bright the lamp should be.
  const float target = targetBrightness(smoothedLight);

  // 3. Fade toward the target at a limited rate (no sudden jumps).
  const float step = (float)TICK_MS / (float)FADE_TIME_MS;
  if (target > brightness) brightness = fminf(brightness + step, target);
  else                     brightness = fmaxf(brightness - step, target);

  // 4. Drive the LEDs.
  const uint32_t duty = brightnessToDuty(brightness);
  pwmWriteAll(duty);

  // 5. Log for calibration / results.
  if (now - lastLog >= LOG_MS) {
    lastLog = now;
    Serial.printf("%lu,%.3f,%.3f,%.3f,%lu\n", (unsigned long)now, smoothedLight,
                  target, brightness, (unsigned long)duty);
  }
}
