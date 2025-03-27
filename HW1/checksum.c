#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/*
XOR Checksum
*/
uint8_t xor(uint8_t message[], int length) {
    // Standard msg exists check
    if (length == 0) {
        return 0;
    }
    uint8_t result = 0; // initialize msg size variable
    for (int i = 0; i < length; i++) {
        result ^= message[i]; // XOR execution
    }
    return result; // return msg size total
}

/*
CCITT16

From CRC file:
   Name   : "CRC-16/CITT"
   Width  : 16
   Poly   : 1021
   Init   : FFFF
   RefIn  : False
   RefOut : False
   XorOut : 0000
   Check  : ?
*/
uint16_t ccitt16(uint8_t message[], int length) {
    // Standard msg exists check
    if (length == 0) {
        return 0;
    }

    uint16_t result = 0xFFFF;          // initial value from above
    uint16_t polynomial = 0x1021;   // CCITT polynomial from abvoe

    for (int i = 0; i < length; i++) {
        // cast from 8 bit to 16 bits and shift 8 bits to hit 16-bit register
        result ^= (uint16_t)message[i] << 8; // XOR Execution 
        for (int bit = 0; bit < 8; bit++) {
            // if 8 bits, realign bits 
            if (result & 0x8000) {
                result = (result << 1) ^ polynomial;
            } else {
                result <<= 1;  // else just shift by 1
            }
        }
    }
    return ~result;
}

int main(int argc, char *argv[]) {
    /*
    argc = command line args
    argv = string array from command line
    */
    // If no bytes are passed, invalid msg and both checksums are 0
    if (argc < 2) {
        printf("0 0\n");
        return 0;
    }

    // Number of input bytes is argc - 1 because program name
    int length = argc - 1;
    uint8_t *message = (uint8_t*)malloc(length * sizeof(uint8_t)); // 8 bit type memory allocation for msg length
    // Check allocation
    if (!message) {
        fprintf(stderr, "Error allocating memory.\n");
        return 1;
    }

    // Parse each command-line argument as a byte (in decimal or hex)
    for (int i = 0; i < length; i++) {
        // argv[0] is program name so start with argv[i + 1]
        message[i] = (uint8_t)strtoul(argv[i + 1], NULL, 0); // string-to-unsigned-long KEEP THIS IN MIND, TOOK AWHILE TO FIND
                    // Other arguments tell strtoul to discover type and convert as needed
                    // Then remember to cast back to 8-bit to pass to checksum functions
                }               


    // Compute checksums
    uint8_t xor_val = xor(message, length);
    uint16_t ccitt_val = ccitt16(message, length);

    // Print results (you can change the format as required by your assignment)
    printf("%u %u\n", xor_val, ccitt_val);

    free(message);
    return 0;
}
