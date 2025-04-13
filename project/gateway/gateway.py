'''Vehicle Gateway'''
import socket
import struct
import time
import sys
from datetime import datetime

"""
This script runs the Vehicle Gateway and connects to the
VAPI container's TCP port 9090 pulling the data from the VAPI.

It also outputs the Vehicle Network status on every data pull.

It reads the 26-byte data struct in the following format with Python conversion:
  uint16_t oil_temp                 << H
  uint16_t maf                      << H
  uint8_t  battery_voltage          << B
  uint16_t tire_pressure            << H
  uint16_t fuel_level               << H
  uint8_t  fuel_consumption_rate    << B
  uint32_t error_codes[4]           << IIII

""" 
STRUCT_FORMAT = "<HHBHHBIIII" # little endian

VEHICLE_STATUS = {  # Can hold more than one vehicle like this but doesn't have to
    "vehicle1": {
        "host": "vapi",   # Docker container name
        "port": 9090,    # vapi/from main.c
        "connected": False,
        "latest_data": {},
        "timestamp": None,
    }
}

def poll_vapi(vehicle_data):
    """
    Connect to VAPI via TCP, read the 26-byte struct, decode, and return as a dict.
    :param vehicle_data  -> 2nd level dict of VEHICLE_STATUS table

    returns updated vehicle_data dict
    """
    host = vehicle_data["host"]
    port = vehicle_data["port"]

    try:
        # Create TCP client socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.settimeout(2.0)

            print(f"[Gateway] Connecting to {host}")
            sock.connect((host, port))
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

            vehicle_data["connected"] = True
            vehicle_data["latest_data"] = {
                "oil_temp": oil_temp,
                "maf": maf,
                "battery_voltage": battery_voltage,
                "tire_pressure_psi": tire_pressure,
                "fuel_level_liters": fuel_level,
                "fuel_consumption_rate": fuel_consumption_rate,
                "error_codes": [hex(err1), hex(err2), hex(err3), hex(err4)]
            }
            vehicle_data["timestamp"] = datetime.now().timestamp()

    except Exception as e:
        vehicle_data["connected"] = False
        print(f"[Gateway] Error {e}, {host} not connected")

        # Return vehicle data dictionary
    return vehicle_data
    
def print_data():
    print("----- Vehicle Network Status -----")
    for v_id, info in VEHICLE_STATUS.items():
        status = "Connected" if info["connected"] else "Disconnected"
        print(f"Vehicle: {v_id}")
        print(f"   Status: {status}")
        if info["latest_data"]:
            ld = info["latest_data"]
            print("   Last Data:")
            print(f"   Time: {info['timestamp']}")
            print(f"       Oil Temp (F): {ld['oil_temp']}")
            print(f"       MAF: {ld['maf']}")
            print(f"       Battery Voltage: {ld['battery_voltage']} V")
            print(f"       Tire Pressure (psi): {ld['tire_pressure_psi']}")
            print(f"       Fuel Level (liters): {ld['fuel_level_liters']}")
            print(f"       Fuel Consumption (l/h): {ld['fuel_consumption_rate']}")
            print(f"       Error Codes: {ld['error_codes']}")
            if len(VEHICLE_STATUS.items()) > 1:
                print("-------")
        else:
            print("   No valid data received yet.")
        print("----------------------------------")

if __name__ == "__main__":
    print("[Gateway] Starting (serial-over-TCP style)...")

    pull_interval = 3
    try:
        while True:
            # get data for all vehicles in table
            try:
                for vehicle_id, vehicle_dict in VEHICLE_STATUS.items():
                    VEHICLE_STATUS[vehicle_id] = poll_vapi(vehicle_dict) # add new vehicle data to table
            except Exception as e:
                print(f"[Gateway] Error while polling VAPI: {e}")
            print_data()
            # Sleep a bit before next poll
            print(f"[Gateway] VAPI pull interval = {pull_interval}s\n")
            time.sleep(pull_interval)
    except KeyboardInterrupt:
        print("\n[Gateway] Interrupt by user")
        sys.exit(0)
