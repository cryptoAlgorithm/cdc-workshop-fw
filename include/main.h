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
