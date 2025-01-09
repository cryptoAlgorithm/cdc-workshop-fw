#pragma once

#include <stdint.h>

/**
 * An ADC sample
 */
typedef struct {
  /**
   * Reading timestamp, in ms
   */
  uint32_t t;
  /**
   * Sample value
   */
  uint16_t val;
} adc_sample_t;

typedef enum {
  MODE_AUTO,
  MODE_HEARTBEAT,
  MODE_GLUCOSE,

  MODE_LAST = MODE_GLUCOSE
} measurement_mode_t;

void change_mode(measurement_mode_t mode, uint8_t inactivity);
