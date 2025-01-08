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
  pinMode(GLUCOSE_BTN_PIN, INPUT_PULLUP);
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
  uint8_t sreg = SREG;
  cli(); // both writes need to be atomic, disable interrupts to ensure ADC ISR doesn't trigger during this time
  sample_channel = new_ch;
  results_available = 0; // make sure only the 
  SREG = sreg;
}

void loop() {
  static adc_sample_t results_proc[READ_BUF_SZ];
  static uint8_t results_count;
  static uint32_t last_glucose_press = 0;

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

    // process everything in a batch
    for (uint8_t i = 0; i < results_count; ++i) {
      process_raw_reading(&results_proc[i]);
      // Serial.println(results_proc[i]);
    }
  }

  const uint32_t now = millis();

  // Check if glucose read button is pressed (500ms debounce)
  if (digitalRead(GLUCOSE_BTN_PIN) == 0 && now - last_glucose_press > 500) { // pullup enabled, button pulls to ground
    // Switch to read glucose sensor (i.e. photoresistor) ADC channel
    adc_update_ch(PRESIST_A_CH);
    Serial.println(F("Reading glucose channel..."));
    while (results_available == 0) {} // wait until new results available
    Serial.println(results[0].val);
    adc_update_ch(PDIODE_A_CH); // update channel to heart rate (clears `results_available` so we do not have to do that before)
    last_glucose_press = now;
  }
}
