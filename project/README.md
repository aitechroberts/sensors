# Vehicle Sensor System
This system is a 5 container system encompassing a Vehicle Sensor API, a Gateway, a Broker, and a Monitoring system consisting of Prometheus and Grafana. 

### Quickstart
Build and Run the system using
```bash
docker compose up --build -d
```

Open the following browser tabs:
`localhost:3000` - login to Grafana with admin/admin
`localhost:9091`  - Prometheus

Create your queries and/or custom dashboards for the metrics to visualize the feedback using the `mqtt_`prefix to metrics seen in [Vehicle API](##vehicle-api)

ChatGPT provided nice little visual:
┌─────────┐   TCP struct   ┌───────────┐  MQTT publish  ┌─────────┐
│  VAPI   │ ─────────────► │  Gateway  │ ──────────────►│ Broker  │
└─────────┘ 26 bytes/req   └───────────┘  topic:value    └─────────┘
      ▲                                         │
      └──────────────── docker network ─────────┘

## Contents
-[Vehicle API](##vehicle-api)
    - [VAPI Overview](###overview)
    - [Data Structure & Simulation](###data-structure--simulation)
    - [Build & Run](###vapi-build--run)
-[Vehicle Gateway](##vehicle-gateway)
    - [Gateway Overview](###gateway-overview)
-[Vehicle Broker](##vehicle-broker)
    -[Broker Selection](###broker-selection)
    -[Broker Overview](###broker-overview)
-[Vehicle Monitoring](##vehicle-monitoring--observability)
    - [Monitoring Architecture](###monitoring-architecture)
    - [Monitoring System](###monitoring-system)
    - [Monitoring Troubleshooting](###monitoring-troubleshooting)


## Vehicle API

This component provides a minimal API providing simulated vehicle data for a Caterpillar Dozer D3K2 written in C, packaged in a Docker container. The API listens on a TCP socket and provides real-time simulated sensor data in a binary format engine and vehicle parameters.

---

### VAPI Overview

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

### VAPI Build & Run

- **Dockerfile:** Describes how to build an Ubuntu-based container and compile the C program.  
- **run.sh:** Optional script to build and run the container with a single command. (Though this project does not have one)
- **vapi/main.c:** The main server code that creates a socket, simulates data, and sends the struct.  
- **vapi/vehicle_api.h:** The packed struct definition and related includes.

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

## Vehicle Gateway

This component provides a minimal Gateway connecting to the Vehicle API above using a serial-over-TCP like pull-based connection packaged in a Docker container. The Gateway polls all "vehicles" with a corresponding, compatible API and outputs the status of those vehicles and their sensor data every 3 seconds.


### Gateway Overview
The gateway.py python script uses only standard python packages and creates a predetermined Vehicles Table in a nested dict data structure which could be held elsewhere as a database in say SQLite or the like, but is hardcoded here.
```py
VEHICLE_STATUS = {  # Can hold more than one vehicle like this but doesn't have to
    "vehicle1": {
        "host": "vapi",   # Docker container name
        "port": 9090,    # vapi/from main.c
        "connected": False,
        "latest_data": {},
        "timestamp": None,
    }
}
```

It then makes a TCP connection to the Vehicles in the table providing their APIs are connected and printing an error if not. Upon successful connection and data retreival, it decodes the data using `struct.unpack`, and then updates the given Vehicle's respective data fields.
```python
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
```

Finally, it outputs that data for every vehicle in the network which will look like the following if using Docker Compose
```bash
gateway  | [Gateway] Connecting to vapi
gateway  | [Gateway] Connection established. Reading data...
gateway  | ----- Vehicle Network Status -----
gateway  | Vehicle: vehicle1
gateway  |    Status: Connected
gateway  |    Last Data:
gateway  |    Time: 1744513880.344433
gateway  |        Oil Temp (F): 185
gateway  |        MAF: 1489
gateway  |        Battery Voltage: 9 V
gateway  |        Tire Pressure (psi): 93
gateway  |        Fuel Level (liters): 82
gateway  |        Fuel Consumption (l/h): 15
gateway  |        Error Codes: ['0x0', '0x0', '0x55', '0x0']
gateway  | ----------------------------------
gateway  | [Gateway] VAPI pull interval = 3s
```

## Vehicle Broker
This component provides an open-source, trusted, lightweight message broker using the MQTT protocol called Eclipse Mosquitto to provide message brokering inside the Vehicle Sensor System.

### Broker Selection
The Eclipse Mosquitto was chosen after a quick google serach for 'best mqtt brokers for iot'.Mosquitto was built specifically for embedded systems to facilitate machine to machine communication under low-bandwidth. It can handle thousands of connections with basic security mechanisms along with a message store feature for when services go offline making it ideal for this system, but would not be proper to use to in a large-scale deployment that requires advanced security capabilities. It also uses command line tools rather than a GUI as requested by professor. The following sources were used to select the MQTT broker:

- [Mosquitto: Pros/Cons](https://www.emqx.com/en/blog/mosquitto-mqtt-broker-pros-cons-tutorial-and-modern-alternatives)
- [Mosquitto: Comparison](https://marsbased.com/blog/2024/02/13/comparison-of-mqtt-brokers-for-iot)
- [Documentation](https://mosquitto.org/documentation/)
- [Docker Docs](https://hub.docker.com/_/eclipse-mosquitto)
- [Example Implementation](https://github.com/sukesh-ak/setup-mosquitto-with-docker)
- [In Depth Implementation](https://cedalo.com/blog/mosquitto-docker-configuration-ultimate-guide/)

### Broker Overview
The broker consists of the Dockerfile that pulls over the official mosquitto image, a logger.sh file that subscribes to and prints everything in lieu of a persistent data store, and a mosquitto.conf file that identifies the listener and logs everything to stdout as per rubric request.


## Vehicle Monitoring & Observability
This component stack adds **Prometheus + Grafana** on top of the broker to satisfy every Functional and Non-Functional requirement in the rubric providing a user friendly interface to continuously monitor the vehicle metrics.

### Monitoring Architecture
```text
            ┌────────────┐   MQTT   ┌─────────┐
            │  Gateway   │─────────►│ Broker  │
            │  (Python)  │          └─────────┘
            │            │               │
            │  mqtt-exporter            (scrape :9000)
            └──────┬─────┘               │
                   ▼                     ▼
            ┌────────────┐  HTTP  ┌────────────┐
            │ Prometheus │◄───────│  Grafana   │
            └────────────┘        └────────────┘
```

### Monitoring System
MQTT-Exporter converts MQTT topics published from the Gateway to the Broker into Prometheus metrics (mqtt_oil_temp, mqtt_battery_voltage, …). Prometheus scrapes those metrics every 15 seconds and stores them in its time-series database while Grafana visualizes the live and/or historical data depending on how the user configures the dashboard.

Prometheus Visual:
![alt text](image.png)

Grafana Visual:
![alt text](image-1.png)
![alt text](image-2.png)

### Monitoring Troubleshooting
If Prometheus or Grafana are not receiving the metrics to create the visualizations you can put the following commands in to terminal to ensure data communication.

```bash
# 1 MQTT flow is visible
docker exec -it broker mosquitto_sub -v -t 'vehicle/#'

# 2 Exporter emits samples
curl http://localhost:9000/metrics | grep mqtt_oil_temp | head

# 3 Prometheus target is UP
curl -s http://localhost:9091/api/v1/targets | jq '.data.activeTargets[] |
     select(.labels.job=="mqtt-exporter") | {scrapeUrl,health}'
```