// Contributor, brother who is a software eng. Helped me build the server functionality
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "vehicle_api.h"

// Hardcode a listening port (could also parse from env or argv)
#define SERVER_PORT 9090

// Initialize Vehicle Data structure, but keep in mind how this would be fed by sensors
static VehicleData global_vehicle_data;

// Simulate or update the data at fixed intervals
static void simulate_vehicle_data(VehicleData* vdata) {
    /*
    I thought about doing a gaussian distribution here for fun and to create more realistic
    data, but then, I figured obviously we need to be able to get error codes often for the project.

    If you read this though and like the idea, let me know, and I'll implement it.
    
    CATERPILLAR D3K2 DOZER
    */
    static int toggle = 0;
    toggle = !toggle;

    // Oil temp: range ~180F to 220F
    vdata->oil_temp = 180 + (rand() % 40);

    // Mass Air Flow: 0-2047 raw => 0-2500 cfm, linear scale
    uint16_t raw_maf = (uint16_t)(rand() % 2048);
    float maf_cfm = ((float)raw_maf / 2047.0f) * 2500.0f;
    vdata->maf = (uint16_t)maf_cfm; // store as integer cfm

    // Battery voltage: 0..12
    vdata->battery_voltage = (uint8_t)(rand() % 13);

    // Tire pressure: also assume 0..2047 for raw A/D
    uint16_t raw_tire = (uint16_t)(rand() % 2048);
    float tire_psi = ((float)raw_tire / 2047.0f) * 100.0f;
    vdata->tire_pressure = (uint16_t)tire_psi; // store as integer psi

    // Fuel level: Max 195 Liters
    vdata->fuel_level = (uint8_t)rand() % 196;

    // Fuel consumption: low/medium/high liters/hr
    int usage_mode = rand() % 3; // 0 => Low, 1 => Medium, 2 => High
    float fuel_consumption;
    switch (usage_mode) {
        case 0: // Low 
            fuel_consumption = 7.6f; 
            break;
        case 1: // Medium 
            fuel_consumption = 8.7f;
            break;
        default: // 2 => High 
            fuel_consumption = 15.5f;
            break;
    }
    // Cast to 8-bit integer
    vdata->fuel_consumption_rate = (uint8_t)fuel_consumption;

    // Simulate error codes; set them all to 0 except one random
    for(int i = 0; i < 4; i++) {
        vdata->error_codes[i] = 0;
    }
    // Example: set one error code to a random known code
    // A small set of random known codes, e.g. 0xA1, 0xC1, 0x55, 0x23
    switch(rand() % 4) {
        case 0: {
            int cylinder = (rand() % 8) + 1; // cylinder range 1..8
            // 0xA0 + 1 => 0xA1, 0xA0 + 2 => 0xA2, etc.
            vdata->error_codes[0] = (uint32_t)(0xA0 + cylinder);
        } break; // Misfire Cylinder 1
        case 1: {
            int cylinder = (rand() % 8) + 1; // cylinder range 1..8
            // 0xA0 + 1 => 0xA1, 0xA0 + 2 => 0xA2, etc.
            vdata->error_codes[1] = (uint32_t)(0xC0 + cylinder);
        } break; // Fuel Injector 1
        case 2: vdata->error_codes[2] = 0x55; break; // Low Oil Pressure
        case 3: vdata->error_codes[3] = 0x23; break; // Low Coolant
    }
}

int main(void) {
    // Over commenting server because I've never done this, and want to have a good personal reference.
    srand((unsigned int)time(NULL)); // initiate random seed

    // Initialize the global data at 0 so that random memory junk doesn't initiate weird values
    memset(&global_vehicle_data, 0, sizeof(global_vehicle_data));

    /*
    CREATE IPv4 TCP Socket:
    AF_INET - socket parameter to use IPv4 protocol
    SOCK_STREAM - socket parameter to create a streaming socket
    0 - socket parameter to use default protocols for the above which is TCP
    */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return 1;
    }

    /*
    Enable a Reuse Address to avoid errors when restarting
    SO_REUSEADDR parameter tells OS to allow immediate resuse of port if server restarts
    - Basically rebinding to the port without error
    */ 
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    /*
    BIND TO PORT
    sockaddr_in - struct holding message header information like:
        family- in this case AF_INET = IPv4
        IP ADDRESS - in this case INADDR_ANY means accept connections on any local network interface
        htons(SERVER_PORT) -  sets port to listen on and converts "host byte order" to "network byte order"
    */
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr)); // same memory safe initialization
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(SERVER_PORT);

    // Bind port to socket
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        close(sockfd);
        return 1;
    }

    // Listen for incoming connections with availability of 5 sockets
    if (listen(sockfd, 5) < 0) {
        perror("ERROR on listen");
        close(sockfd);
        return 1;
    }

    printf("Vehicle API listening on port %d...\n", SERVER_PORT);

    // Our main server loop
    while (1) {
        // Update the data every iteration
        simulate_vehicle_data(&global_vehicle_data);

        /*
        ACCEPT NEW CONNECTIONS
        Use cli_addr in same structure like sockaddr_in to accept new clients
        Establish new socket for cli_addr at size of clilen
        */
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            // No incoming connection found yet, sleep briefly and keep looping
            // In production, you might want select() or poll() for event-driven
            usleep(200000); 
            continue;
        }

        // WWith new socket, send the global_vehicle_data message over this socket
        ssize_t written = write(newsockfd, &global_vehicle_data, sizeof(global_vehicle_data));
        if (written < 0) {
            perror("ERROR writing to socket");
        } else {
            printf("Sent %zd bytes of vehicle data.\n", written);
        }

        close(newsockfd);
    }

    // Clean up (unreachable in this example)
    close(sockfd);
    return 0;
}
