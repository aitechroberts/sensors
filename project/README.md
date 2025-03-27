# Vehicle Sensor System

## Contents
-[Vehicle API](##vehicle-api)
    - [Overview](###overview)
    - [Data Structure & Simulation](###data-structure--simulation)
    - [Build & Run](###build--run)

## Vehicle API

This project provides a minimal API providing simulated vehicle data for a Caterpillar Dozer D3K2 written in C, packaged in a Docker container. The API listens on a TCP socket and provides real-time simulated sensor data in a binary format engine and vehicle parameters.

---

### Overview

- **Language:** C  
- **Purpose:** Provide a simple server that simulates essential vehicle metrics (oil temperature, tire pressure, mass air flow, fuel level, fuel consumption rate, battery voltage, and error codes) to allow other components (e.g., a Gateway or client applications) to retrieve this data over a network connection.

---

### Data Structure & Simulation

The API follows best practice using a packed struct using pragma called `VehicleData` to serve the data which ensures consistent byte layout across different platforms. VehicleData parameters:

- **oil_temp** (`int16_t`): engine oil temperature in °F  
- **maf** (`uint16_t`): mass air flow in cfm (converted from an 11-bit raw reading)  
- **battery_voltage** (`uint8_t`): battery voltage in volts, 0–12  
- **tire_pressure** (`uint16_t`): tire pressure in psi (converted from an 11-bit raw reading)  
- **fuel_level** (`uint16_t`): fuel level in liters  
- **fuel_consumption_rate** (`uint8_t`): approximate liters/hour consumption  
- **error_codes[4]** (`uint32_t`): array of four 32-bit fields to hold 8-bit diagnostic codes (e.g., 0xA1, 0xC1, etc.)

The server sends this struct in **binary** form whenever a client connects and is simulated for each parameter as follows:

1. **Oil Temperature:** Randomized around 180–220°F.  
2. **Mass Air Flow (MAF):** Another 11-bit range (0–2047) linearly scaled to 0–2500 cfm.  
3. **Battery Voltage:** Random 0–12 (uint8_t).  
4. **Tire Pressure:** Simulates a raw 0–2047 (11-bit) reading and linearly scales to 0–100 psi.  
5. **Fuel Level:** Random 0–500 liters.  
6. **Fuel Consumption:** Uses a simple model for a dozer (D3K23) with “low,” “medium,” or “high” usage scenarios, multiplied by a random load factor in each range.  
7. **Error Codes:** Picks one random code among:  
   - **Misfire:** 0xA1–0xA8 (random cylinder 1–8)  
   - **Fuel Injector:** 0xC1–0xC8 (random cylinder 1–8)  
   - **Low Oil Pressure:** 0x55  
   - **Low Coolant:** 0x23  

---

### Build & Run

- **Dockerfile:** Describes how to build an Ubuntu-based container and compile the C program.  
- **run.sh:** Optional script to build and run the container with a single command.  
- **src/main.c:** The main server code that creates a socket, simulates data, and sends the struct.  
- **src/vehicle_api.h:** The packed struct definition and related includes.

---

#### Requirements

- [Docker](https://docs.docker.com/engine/install/) installed  
- A POSIX-like shell (macOS, Linux, or Windows WSL/MinGW with Bash)

---
#### Manual Steps

1. Clone the repository and navigate to the project folder
2. Build the Docker container
```bash
docker build -t vehicle_api_image .
```
3. Run the Docker container
```bash
docker run -d --name vehicle_api_container -p 9090:9090 vehicle_api_image
```

### Testing
You may use netcat to receive a simple binary dump as soon as the API receives the connection as in this test example

Request data from the API in Hexadecimal using: `nc 127.0.0.1 9090 | xxd`

Output will be a variation of the following:
```bash
00000000: be00 c006 0922 0060 000f a800 0000 0000  .....".`........
```
