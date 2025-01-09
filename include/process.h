#pragma once

#include <stdint.h>

#include "main.h"

/**
 * Init internal parameters for hb reading after mode switch
 */
void process_init_hb();

/**
 * Process one photodiode reading
 */
void process_raw_reading(adc_sample_t *);

/**
 * Init internal parameters for glucose reading after mode switch
 */
void process_init_glucose();

void process_glucose_reading(adc_sample_t * sample);
