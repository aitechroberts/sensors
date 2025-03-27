#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
Goal of program is to convert 11-bit ADC counts (0..2047) to PSI.
    ** Returns 0xFFFF if out-of-range.
    16-bit ADC reading provided in terminal; (only lower 11 bits valid).
Returns uint16_t PSI value or 0xFFFF.
*/

uint16_t psi(uint16_t counts) {
  // Check range
  if (counts > 2047) {
    return 0xFFFF; // Error condition
  }

  // Knot 1: 0 to 585 counts → 0 to 180 psi
  if (counts <= 585) {
    // psi = (180 * counts) / 585
    // Use 32-bit intermediate to avoid overflow in multiply
    uint32_t temp = (uint32_t)counts * 180;
    uint16_t result = (uint16_t)(temp / 585);
    return result;
  }
  // Knot 2: 586 to 2047 counts → 180 to 300 psi
  else {
    // psi = 180 + ((counts - 585) * 120 / (2047-585))
    uint32_t diff = (uint32_t)(counts - 585) * 120;
    uint16_t result = 180 + (uint16_t)(diff / (2047 - 585));
    return result;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <adc_counts>\n", argv[0]);
    return 1;
  }

  // Read input, calculate pressure, and print
  uint16_t counts = (uint16_t)atoi(argv[1]);
  uint16_t pressure = psi(counts);
  if (pressure == 0xFFFF) {
    printf("ADC counts (%u) is out of valid range. Return = 0xFFFF.\n", counts);
  } else {
    printf("ADC counts = %u -> Pressure = %u psi\n", counts, pressure);
  }

  return 0;
}
