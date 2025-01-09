#include "main.h"

#include <Arduino.h>

#include "pins.h"
#include "lcd.h"

#include "process.h"

// ADC configuration
#define READ_BUF_SZ 72
// Number of ADC conversions to average (oversample) for one sample
// One ADC conversion takes 13 ADC clk cycles, i.e. conversion rate of ~9.6kHz with 125kHz ADC clk
// Oversampling of 256 results in sample rate of ~38Hz
#define READ_OVERSAMPLE 256

void setup() {
  // Configure ADC peripheral
  // According to datasheet pg. 208, max resolution achieved with ADC clk 50-200kHz, so we pick 125kHz
  ADCSRA = 1<<7 | 1<<3 | 0b111; // enable adc peripheral, conversion interrupt (ADEN=1, ADSC=1, prescaler=128)
  ADMUX = 1<<6; // config mux: REFS = 0b01 (AVCC), ADLAR = 0
  DIDR0 = 0b11; // disable digital buffers on ADC0 and 1 (datasheet pg. 120)

  // Configure IO
  pinMode(MODE_POT_PIN, INPUT);
  pinMode(HB_LED_PIN, OUTPUT);

  // Init Serial
  Serial.begin(500000);
  Serial.println(F("Begin!"));

  // Init LCD
  lcd_init();

  // Start conversion
  Serial.println(F("Start ADC conversion..."));
  sei(); // enable interrupts
  ADCSRA |= 1<<6; // ADSC = 1

  change_mode(MODE_AUTO, 0);
}

//
volatile uint32_t sum = 0;
volatile adc_sample_t results[READ_BUF_SZ];
volatile uint8_t results_available = 0;
volatile uint16_t num_readings = 0;
volatile uint8_t sample_channel = PDIODE_A_CH;

/**
 * ADC sample complete ISR
 */
ISR(ADC_vect) { // a conversion has just completed
  // Check if requested channel has been changed
  if ((ADMUX & 0b1111) != sample_channel) { // MUX[3:0] value differs; update channel
    ADMUX = (ADMUX & ~(0b1111)) | sample_channel; // update ADC MUX channel
    results_available = num_readings = sum = 0; // reset vars, discard pending result
  } else {
    sum += ADC; // `ADC` register contains conversion result, read and add to sum
    ++num_readings;
    if (num_readings > READ_OVERSAMPLE) { // acquired sufficient samples
      if (results_available < READ_BUF_SZ) { // have space in output buffer
        results[results_available].val = sum/READ_OVERSAMPLE;
        results[results_available].t = millis();
        ++results_available;
      }
      sum = num_readings = 0;
    }
  }

  // Start another conversion
  ADCSRA |= 1<<6; // ADSC = 1
  // TODO: Maybe use free-running auto trigger method instead?
}

/**
 * Update the ADC channel used for measurements
 * 
 * Note:
 * Because an ADC conversion is probably ongoing at the moment, delegate
 * changing the mux channel to the ADC ISR.
 * A conversion cannot be cancelled nor can the mux channel be changed midway.
 */
static void adc_update_ch(uint8_t new_ch) {
  if (new_ch == sample_channel) return; // no change; don't need to do anything
  uint8_t sreg = SREG;
  cli(); // both writes need to be atomic, disable interrupts to ensure ADC ISR doesn't trigger during this time
  sample_channel = new_ch;
  results_available = 0; // make sure only the 
  SREG = sreg;
}


static measurement_mode_t cur_mode = MODE_AUTO;
static uint32_t last_mode_change = 0;

void change_mode(measurement_mode_t mode, uint8_t inactivity) {
  const char * title;
  switch (mode) {
    case MODE_AUTO:
      title = inactivity ? "Return to auto" : "Automatic";
      break;
    case MODE_HEARTBEAT:
      title = "Heartbeat";
      adc_update_ch(PDIODE_A_CH); // ensure correct ADC channel set (noop if already correct)
      process_init_hb();
      break;
    case MODE_GLUCOSE:
      title = "Glucose";
      adc_update_ch(PRESIST_A_CH);
      process_init_glucose();
      break;
  }
  lcd_draw_alert(inactivity ? "User Inactivity" : "Current Mode", title);
  last_mode_change = millis();
  cur_mode = mode;
}

static uint8_t home_anim_seq = 0;

static void render_initial_mode(measurement_mode_t mode) {
  lcd.home();
  switch (mode) {
    case MODE_AUTO:
      lcd_draw_text_center("Automatic Mode", 0, 0);
      lcd.setCursor(0, 1); lcd.print(F("Touch sensor or"));
      lcd.setCursor(0, 2); lcd.print(F("insert cuvette to"));
      lcd.setCursor(0, 3); lcd.print(F("start measurement"));
      // Outline for animation
      lcd.setCursor(18, 1); lcd.write('|');
      lcd.setCursor(18, 2); lcd.write('|');
      lcd.setCursor(18, 3); lcd.write(LCD_CHAR_BOTTOM_LEFT);
      lcd.setCursor(19, 3); lcd.write('-');
      home_anim_seq = 0;
      break;
    case MODE_HEARTBEAT:
      lcd_draw_text_center("Heartrate Monitor", 0, 0);
      lcd.setCursor(0, 1); lcd.print(F("Reading..."));
      break;
    case MODE_GLUCOSE:
      lcd_draw_text_center("Glucose Monitor", 0, 0);
      // Cuvette graphic
      lcd.setCursor(17, 1); lcd.write('|');
      lcd.setCursor(17, 2); lcd.write('|');
      lcd.setCursor(17, 3); lcd.write(LCD_CHAR_BOTTOM_LEFT);
      lcd.setCursor(18, 3); lcd.write('-');
      lcd.setCursor(19, 1); lcd.write('|');
      lcd.setCursor(19, 2); lcd.write('|');
      lcd.setCursor(19, 3); lcd.write(LCD_CHAR_BOTTOM_RIGHT);
      break;
  }
}

static void render_home_anim() {
  switch (home_anim_seq) {
    case 0:
      lcd.setCursor(19, 0); lcd.write(' ');
      lcd.setCursor(19, 1); lcd.write(' ');
      lcd.setCursor(19, 2); lcd.write(' ');
      break;
    case 1:
      lcd.setCursor(19, 0); lcd.write(0xff); // block
      break;
    case 2: 
      lcd.setCursor(19, 0); lcd.write(' ');
      lcd.setCursor(19, 1); lcd.write(0xff);
      break;
    case 3: 
      lcd.setCursor(19, 1); lcd.write(' ');
      lcd.setCursor(19, 2); lcd.write(0xff);
      break;
    case 4: 
      lcd.setCursor(19, 0); lcd.write(0xff);
      break;
    case 5: 
      lcd.setCursor(19, 0); lcd.write(' ');
      lcd.setCursor(19, 1); lcd.write(0xff);
      break;
    case 6:
      lcd.setCursor(19, 0); lcd.write(LCD_CHAR_TICK);
      break;
    case 7:
      break;
  }
  ++home_anim_seq;
  if (home_anim_seq > 7) home_anim_seq = 0;
}

void loop() {
  static adc_sample_t results_proc[READ_BUF_SZ];
  static uint8_t results_count;
  static uint8_t cur_mode_pos = digitalRead(MODE_POT_PIN);
  static uint32_t last_anim = 0;

  const uint32_t now = millis();

  if (lcd_can_draw()) {
    if (cur_mode == MODE_AUTO && now - last_anim > 800) {
      render_home_anim();
      last_anim = now;
    }
  } else if (now - last_mode_change > 1000) {
    lcd_clear();
    render_initial_mode(cur_mode);
  }

  if (results_available) { // wait until at least one result is available in the buffer
    // process the batch of ADC readings
    const uint8_t sreg = SREG;
    cli(); // disable interrupts for atomicity. note that interrupts received aren't discarded, they just aren't serviced
    // copy to another buffer for processing that won't be mutated by ISR (memcpy not used as it discards volatile modifier)
    for (uint8_t i = 0; i < results_available; ++i) {
      results_proc[i].t = results[i].t;
      results_proc[i].val = results[i].val;
    }
    // update counters for our use and ISR
    results_count = results_available;
    results_available = 0;
    SREG = sreg; // re-enable interrupts

    if (lcd_can_draw()) {
      // first, check the current mode
      if (cur_mode == MODE_AUTO) {
        if (sample_channel == PDIODE_A_CH && results_proc[0].val < 10) { // reading sharply falls to 0 when finger first placed
          // probably have a finger
          change_mode(MODE_HEARTBEAT, 0);
          Serial.print(F("Switch to heartbeat: "));
          Serial.println(results_proc[0].val);
        } else if (sample_channel == PRESIST_A_CH && results_proc[0].val < 150) {
          change_mode(MODE_GLUCOSE, 0);
          Serial.print(F("Switch to glucose: "));
          Serial.println(results_proc[0].val);
        } else {
          adc_update_ch(sample_channel == PDIODE_A_CH ? PRESIST_A_CH : PDIODE_A_CH);
        }
        // switch between reading both sensors for autodetection
      } else if (cur_mode == MODE_HEARTBEAT) {
        for (uint8_t i = 0; i < results_count; ++i) { // process everything in the batch
          process_raw_reading(&results_proc[i]);
          // Serial.println(results_proc[i].val);
        }
      } else if (cur_mode == MODE_GLUCOSE) {
        process_glucose_reading(&results_proc[0]);
      }
    }
  }

  // Check if mode pot position changed
  uint8_t cur_pot_pos = digitalRead(MODE_POT_PIN);
  if (cur_pot_pos != cur_mode_pos && lcd_can_draw()) {
    change_mode(cur_mode >= MODE_LAST ? MODE_AUTO : (measurement_mode_t) (cur_mode+1), 0);
    last_mode_change = now;
    cur_mode_pos = cur_pot_pos;
  }
}
