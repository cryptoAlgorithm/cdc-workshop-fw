#pragma once

#include <stdint.h>

#include "main.h"

/**
 * Process one photodiode reading
 */
void process_raw_reading(adc_sample_t *);
