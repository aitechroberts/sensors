#include <stdint.h>
#include <stdio.h>
#include <string.h>

/** init
 * Initialize the pointer to the client code.
 */
void (*callback)(int *);
void init(void (*ptr)(int *)) { callback = ptr; }

/** transmit_bytewise
 * Transmit the array back to the python harness
 * one byte at a time.  This is similar to how bytes
 * are 'clocked out' of a microcontroller.
 */
void transmit_array(int array[], int length) {
  for (int i = 0; i < length; i++) {
    (*callback)(&array[i]);
  }
}

/** transmit
 * Call this to transmit a value to the controller.
 * Note that there are no enforcement mechanisms to ensure
 * that the pointer value was set.
 */
void transmit(int value) { (*callback)(&value); }

/** process
 * This function receives values from the test harness.
 * The necessity of this is to use a signed integer, and
 * for program 2, the value should be cast to unit16_t.
 * This demo program will execute on the harness's thread.
 * We're simply taking the input value from the harness
 * and echoing it right back over the callback into the harness.
 */
void process(int value) { transmit(value); }

/** receive
 * This function takes in arbitrary sized arrays from
 * the test harness.  As before, cast elements in the
 * message to uint16_t before using them.
 */
void receive(int message[], int length) { receive_all(message, length); }

// -----------------------------------------------------
// Everything above this is to interact with autograder except receive
// because it's compiled at the bottom
// -----------------------------------------------------

// Default device ID is 0x000A
#define DEVICE_ID_HIGH 0x00
#define DEVICE_ID_LOW 0x0A
#define EOL_BYTE 0x10 // Delimiter byte

#define PARSE_BUFFER_SIZE                                                      \
  512 // 1 message could be up to 256 bytes so double that to start and adjust
      // as necessary for message buffering
static uint8_t parseBuffer[PARSE_BUFFER_SIZE];
static int parseCount = 0;

// XOR CRC Checksum
static uint8_t crc_checksum(const uint8_t *data, int length) {
  uint8_t crc = 0;
  for (int i = 0; i < length; i++) {
    crc ^= data[i];
  }
  return crc;
}

// Build and send response message
// Default Format: [Length][CRC][DevHigh][DevLow][Payload...][EOL][EOL]
static void send_response(const uint8_t *payload, int payload_len) {
  // total length in bytes = 1 (Len) + 1 (CRC) + 2 (DevID) + payload_len + 2
  // (EOL,EOL)
  int total_len = 1 + 1 + 2 + payload_len + 2;
  int msg_out[256]; // Enough for max size

  // The length field is the count of all bytes after 'Length'
  int length_field = total_len - 1;

  // Neat idea on index. initialize at 0 and increment value inside array
  // assignment as you go Found online, and basically means assign index in
  // array THEN increment value but one-liner
  int idx = 0;
  msg_out[idx++] = length_field; // [Length]
  int crc_index = idx;           // We'll fill [CRC] later
  msg_out[idx++] = 0x00;         // placeholder for CRC

  // Device ID
  uint16_t deviceId = 0x000A;
  msg_out[idx++] = (uint8_t)((deviceId >> 8) & 0xFF); // high byte
  msg_out[idx++] = (uint8_t)(deviceId & 0xFF);        // low byte

  // Copy message payload
  for (int i = 0; i < payload_len; i++) {
    msg_out[idx++] = payload[i];
  }

  // EOL x2
  msg_out[idx++] = EOL_BYTE;
  msg_out[idx++] = EOL_BYTE;

  // Now compute the XOR-based CRC over everything *after* the CRC field
  // Test Harness expects int, and casting to msg_out to uint8_t is problematic
  uint8_t crc_val = 0;
  for (int i = crc_index + 1; i < idx; i++) {
    crc_val ^= (uint8_t)msg_out[i];
  }
  msg_out[crc_index] = crc_val;

  // Send the response, one integer at a time
  transmit_array(msg_out, idx);
}

// Handle the valid message
static void handle_message_payload(const uint8_t *payload, int payload_len) {
  if (payload_len < 1) {
    // No command byte? Ignore.
    return;
  }
  uint8_t command = payload[0];

  // Respond to PING (0x01) with token+1
  if (command == 0x01) {
    // Must have at least 2 bytes: [0x01, token]
    if (payload_len < 2) {
      // If there's only 1 byte (the command), we might treat it as invalid
      // or we might silently ignore. Up to you; some tests require a response.
      return;
    }
    uint8_t token = payload[1];

    // We'll respond with the same payload, except we increment payload[1].
    // So the response payload size is the same as the incoming payload size.
    uint8_t response[256];
    response[0] = 0x01;      // same command
    response[1] = token + 1; // increment the token

    // If there's extra data, copy it from payload[2..end]
    for (int i = 2; i < payload_len; i++) {
      response[i] = payload[i];
    }

    // total payload length = same as incoming
    send_response(response, payload_len);
    return;
  }
  // Otherwise, unknown command → respond with command=0xFF, echo entire payload
  else {
    uint8_t response[256];
    response[0] = 0xFF; // command first
    for (int i = 0; i < payload_len; i++) {
      response[1 + i] = payload[i];
    }
    send_response(response, 1 + payload_len);
  }
}

// Parse all complete messages from parseBuffer
static void parse_messages(void) {
  int offset = 0;

  while (offset + 1 <= parseCount) {
    // First byte is the length field
    uint8_t lengthField = parseBuffer[offset];

    // Total message size is 1 + lengthField
    int needed = 1 + lengthField;

    if (offset + needed > parseCount) {
      // bogus length or incomplete data
      offset += 1; // skip one byte
      continue;
    }

    // Indices within the buffer
    int cursor = offset + 1; // position of CRC
    if (cursor >= PARSE_BUFFER_SIZE)
      break;

    uint8_t msgCRC = parseBuffer[cursor++];
    if (cursor + 1 >= PARSE_BUFFER_SIZE)
      break;

    uint16_t devId = ((uint16_t)parseBuffer[cursor] << 8) |
                     ((uint16_t)parseBuffer[cursor + 1]);
    cursor += 2;

    // The payload length = needed - 1 (Len field) - 1 (CRC) - 2 (DevID) - 2
    // (EOL/EOL)
    int payload_len = needed - 1 - 1 - 2 - 2;
    if (payload_len < 0) {
      offset += 1;
      continue;
    }

    int payloadStart = cursor;
    int payloadEnd = cursor + payload_len - 1;
    int eol1 = payloadEnd + 1;
    int eol2 = payloadEnd + 2;

    // Validate we have space for EOLs
    if (eol2 >= PARSE_BUFFER_SIZE) {
      offset += 1;
      continue;
    }

    // Check EOL bytes
    if (parseBuffer[eol1] != EOL_BYTE || parseBuffer[eol2] != EOL_BYTE) {
      offset += 1;
      continue;
    }

    // Compute CRC over everything after msgCRC (devHigh..EOL2)
    // which is parseBuffer[offset+2..offset+needed-1].
    uint8_t calc_data[256];
    int calc_len = 0;
    int start_crc = offset + 2;        // devHigh starts at offset+2
    int end_crc = offset + needed - 1; // last byte in this message

    for (int i = start_crc; i <= end_crc; i++) {
      calc_data[calc_len++] = parseBuffer[i];
    }

    uint8_t calculated_crc = crc_checksum(calc_data, calc_len);

    // Check CRC and devId
    if (calculated_crc != msgCRC || devId != 0x000A) {
      // Invalid CRC or wrong device -> skip 1 byte, keep going
      offset += 1;
      continue;
    }

    handle_message_payload(&parseBuffer[payloadStart], payload_len);

    // Advance offset to next message
    offset += needed;
  }

  // Slide down any unparsed leftover
  if (offset > 0 && offset < parseCount) {
    memmove(parseBuffer, &parseBuffer[offset], parseCount - offset);
    parseCount -= offset;
  } else if (offset == parseCount) {
    parseCount = 0;
  }
}

// --------------------------------------------------------------------

/** receive
 * This function takes in arbitrary sized arrays from
 * the test harness.  As before, cast elements in the
 * message to uint16_t before using them.
 */
void receive_all(int message[], int length) {
  // Accumulate incoming data in parseBuffer
  for (int i = 0; i < length; i++) {
    // Per assignment instructions, cast to uint16_t before use
    uint16_t val = (uint16_t)message[i];
    // Then store as a single byte, if there's space
    if (parseCount < PARSE_BUFFER_SIZE) {
      parseBuffer[parseCount++] = (uint8_t)val;
    }
  }
  // Attempt to parse valid messages
  parse_messages();
}