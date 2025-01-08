#include "process.h"
#include <Arduino.h>

#include "pins.h"

#define HYSTERESIS_THRES 4

static uint32_t cycle_min = 0, cycle_max = 0, frame_min = 10000, frame_max = 11500, last_min = frame_min, last_max = frame_max;
static uint8_t cycle = 1; // 1 = rising, 0 = falling
static uint32_t last_max_time = 0, max_time = 0; // tick of last rise

void process_raw_reading(adc_sample_t * sample) {
  const uint16_t val = sample->val; // value of sample
  const uint32_t now = sample->t; // time sample was completed

  if (val > 700) { // ignore, not enough reflectance
    return;
  }

  if (cycle == 1) { // rising portion of pulse
    if (val > cycle_max) {
      cycle_max = val;
      max_time = now;
    } else if (val < cycle_max - HYSTERESIS_THRES) {
      // now falling!
      const uint32_t diff = max_time - last_max_time; // difference between 2 peaks
      if (diff > 250) { // cap at 240bpm, 60/240 = 250ms
        cycle = 0;
        Serial.print(F("falling, margin: ")); Serial.print(cycle_max - cycle_min);
        Serial.print(F(" BPM: ")); Serial.println(60000 / diff);
        // printf("falling, val: %lu, margin: %lu, time: %lums, bpm: %lu\n", val, cycle_max - cycle_min, diff, 60000 / diff);
        // char textbuf[32];
        // snprintf(textbuf, sizeof(textbuf), "BPM: %lu", 60000 / diff);

        // reset range
        // if (frame_max < cycle_max + 50) {
          last_max = cycle_max + 40;
        // }
        cycle_min = val;
        digitalWrite(HB_LED_PIN, cycle);
        last_max_time = max_time;
      } else {
        // discard this beat
        max_time = now;
      }
    }
  } else { // falling portion of pulse
    if (val < cycle_min) {
      cycle_min = val;
    } else if (val > cycle_min + HYSTERESIS_THRES) {
      // now rising!
      cycle = 1;
      last_min = cycle_min - 40;
      // printf("rising, val: %lu, margin: %lu\n", val, cycle_max - cycle_min);
      Serial.print(F("rising, margin: "));
      Serial.println(cycle_max - cycle_min);
      cycle_max = val;
      digitalWrite(HB_LED_PIN, cycle);
    }
  }
}
