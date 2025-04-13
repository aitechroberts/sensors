'''Vehicle Gateway'''
import socket
import struct
import time

"""
This script runs the Vehicle Gateway and connects to the
VAPI container's TCP port 9090 pulling the data from the VAPI

It reads the 26-byte data struct in the following format:
  uint16_t oil_temp
  uint16_t maf
  uint8_t  battery_voltage
  uint16_t tire_pressure
  uint16_t fuel_level
  uint8_t  fuel_consumption_rate
  uint32_t error_codes[4]

"""

# Configure the IP/port for the VAPI container and struct format
VAPI_HOST = "vapi"  
VAPI_PORT = 9090     # vapi/from main.c
STRUCT_FORMAT = "<HHBHHBIIII" # little endian

def poll_vapi():
    """
    Connect to VAPI via TCP, read the 26-byte struct, decode, and return as a dict.
    """
    # Create TCP client socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(2.0)

        print(f"[Gateway] Attempting to connect to {VAPI_HOST}:{VAPI_PORT}")
        sock.connect((VAPI_HOST, VAPI_PORT))
        print("[Gateway] Connection established. Reading data...")

        # Read 26 bytes vehicle struct
        raw_data = sock.recv(26)
        if len(raw_data) != 26:
            raise ValueError(f"Expected 26 bytes, got {len(raw_data)}")

        # Decode using struct.unpack
        (
            oil_temp,
            maf,
            battery_voltage,
            tire_pressure,
            fuel_level,
            fuel_consumption_rate,
            err1,
            err2,
            err3,
            err4
        ) = struct.unpack(STRUCT_FORMAT, raw_data)

        # Return vehicle data dictionary
        return {
            "oil_temp_f": oil_temp,
            "maf": maf,
            "battery_voltage": battery_voltage,
            "tire_pressure_psi": tire_pressure,
            "fuel_level_liters": fuel_level,
            "fuel_consumption_lh": fuel_consumption_rate,
            "error_codes": [hex(err1), hex(err2), hex(err3), hex(err4)]
        }

if __name__ == "__main__":
    print("Gateway Starting (serial-over-TCP style)...")

    pull_interval = 2
    while True:
        try:
            vehicle_data = poll_vapi()
            print("[Gateway] Successfully read vehicle data:")
            for k, v in vehicle_data.items():
                print(f"   {k}: {v}")
        except Exception as e:
            print(f"[Gateway] Error while polling VAPI: {e}")

        # Sleep a bit before next poll
        print(f"[Gateway] Waiting {pull_interval} seconds before next poll...\n")
        time.sleep(pull_interval)
