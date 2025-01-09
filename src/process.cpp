#include "process.h"
#include <Arduino.h>

#include "lcd.h"

#include "pins.h"

// HR params
#define HYSTERESIS_THRES 300

// Glucose params
#define READING_W 427

// Shared params
#define INACTIVITY_TIMEOUT 10000 // time (in ms) from last valid reading to return to auto mode

static uint16_t cycle_min = 0, cycle_max = 0, last_min = 20, last_max = 1000, last_margin = 0;
static uint8_t cycle = 1; // 1 = rising, 0 = falling
static uint32_t last_max_time = 0, max_time = 0; // tick of last rise
static uint32_t last_graphic_update = 0;
static uint8_t has_finger = 1;

void process_init_hb() {
  has_finger = 1;
  last_max_time = millis();
}

void process_raw_reading(adc_sample_t * sample) {
  const uint16_t val = sample->val; // value of sample
  const uint32_t now = sample->t; // time sample was completed

  if (cycle == 1) { // rising portion of pulse
    if (val > cycle_max) {
      cycle_max = val;
      max_time = now;
    } else if (val < cycle_max - HYSTERESIS_THRES) {
      // now falling!
      const uint16_t margin = cycle_max - cycle_min;
      const uint32_t diff = max_time - last_max_time; // time difference between 2 peaks
      if (diff > 250 && margin > last_margin * 4/5) { // cap at 240bpm, 60/240 = 250ms
        cycle = 0;
        Serial.print(F("falling, margin: ")); Serial.print(margin);
        Serial.print(F(" BPM: ")); Serial.println(60000 / diff);
        // printf("falling, val: %lu, margin: %lu, time: %lums, bpm: %lu\n", val, cycle_max - cycle_min, diff, 60000 / diff);
        // char textbuf[32];
        // snprintf(textbuf, sizeof(textbuf), "BPM: %lu", 60000 / diff);
        if (lcd_can_draw()) {
          if (!has_finger) { // need to clear previous text
            lcd.setCursor(0, 1);
            lcd.print(F("                   "));
          }
          lcd.setCursor(0, 1);
          lcd.print(F("Heart rate: "));
          lcd.print(60000/diff);
          lcd.print(F("BPM  "));
          lcd.setCursor(19, 1);
          lcd.write(LCD_CHAR_HEART_LG);
        }
        has_finger = 1;

        // reset range
        // if (frame_max < cycle_max + 50) {
          last_max = cycle_max + 5;
        // }
        cycle_min = val;
        digitalWrite(HB_LED_PIN, cycle);
        last_max_time = max_time;
      } else {
        // discard this beat
        max_time = now;
      }
      last_margin = margin;
    }
  } else { // falling portion of pulse
    if (val < cycle_min) {
      cycle_min = val;
    } else if (val > cycle_min + HYSTERESIS_THRES) {
      // now rising!
      const uint16_t margin = cycle_max - cycle_min;

      if (margin > last_margin * 4/5) {
        cycle = 1;
        last_min = cycle_min;
        // printf("rising, val: %lu, margin: %lu\n", val, cycle_max - cycle_min);
        Serial.print(F("rising, margin: "));
        Serial.println(margin);
        cycle_max = val;
        digitalWrite(HB_LED_PIN, cycle);

        if (lcd_can_draw()) {
          lcd.setCursor(19, 1);
          lcd.write(LCD_CHAR_HEART_SM);
        }
      } else {
        last_margin = margin;
      }
    }
  }

  uint32_t real_now = millis();
  if (real_now - last_graphic_update > 50 && lcd_can_draw()) {
    uint8_t rescale_val = (val - last_min) * 20 / (last_max - last_min);
    Serial.print(F("val: "));
    Serial.print(val);
    Serial.print(F(", rescale: "));
    Serial.println(rescale_val);
    for (uint8_t x = 0; x < 20; ++x) {
      lcd.setCursor(x, 3);
      lcd.write(x <= rescale_val ? 0xff : ' ');
    }
    last_graphic_update = real_now;

    if (now - last_max_time > 3000 && has_finger) { // no beat for at least 3s -> probably no finger
      if (val == 0) {
        lcd.setCursor(0, 1);
        lcd.print(F("Reading..."));
      } else {
        has_finger = 0;
        lcd.setCursor(0, 1);
        lcd.print(F("Please touch sensor"));
      }
    }

    if (now - last_max_time > INACTIVITY_TIMEOUT) {
      change_mode(MODE_AUTO, 1);
    }
  }
}

static uint32_t last_glucose_valid_time = 0;

void process_init_glucose() {
  last_glucose_valid_time = millis();
}

void process_glucose_reading(adc_sample_t * sample) {
  if (!lcd_can_draw()) return;

  const uint16_t val = sample->val;
  const uint32_t now = millis();
  if (val > 170) { // probably no cuvette inserted
    if (now - last_glucose_valid_time > INACTIVITY_TIMEOUT) {
      change_mode(MODE_AUTO, 1);
      return;
    }

    lcd.setCursor(18, 1); lcd.write(' ');
    lcd.setCursor(18, 2); lcd.write('?');

    lcd.setCursor(0, 2); lcd.print(F("Please insert"));
    lcd.setCursor(0, 3); lcd.print(F("cuvette."));
  } else {
    lcd.setCursor(18, 1); lcd.write(0xff);
    lcd.setCursor(18, 2); lcd.write(0xff);

    double conc = (log10((double) READING_W / (double) val)-0.0224) / 0.0335;

    lcd.setCursor(0, 2);
    lcd.print(F("Conc: "));
    lcd.print(conc);
    lcd.print(F("mM    "));
    lcd.setCursor(0, 3);
    lcd.print(F("               "));

    last_glucose_valid_time = now;
  }
}
