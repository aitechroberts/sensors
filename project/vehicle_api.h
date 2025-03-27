#ifndef VEHICLE_API_H
#define VEHICLE_API_H

#include <stdint.h>

// Pack the struct tightly to avoid any compiler-inserted padding.
// Seems to be the de facto best method to keep this compact on an embedded system and to keep the bytes predictable
#pragma pack(push, 1)

typedef struct {
    uint16_t  oil_temp;              // degrees F
    uint16_t maf;                   // raw sensor reading (11-bit range)
    uint8_t  battery_voltage;       // 0-12 volts
    uint16_t tire_pressure;         // raw sensor reading (11-bit range)
    uint16_t fuel_level;            // liters
    uint8_t  fuel_consumption_rate; // liters/hour
    uint32_t error_codes[4];        // 4 8-bit codes packed into four 32-bit fields
} VehicleData;

#pragma pack(pop)

#endif // VEHICLE_API_H
