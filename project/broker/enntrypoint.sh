#!/bin/sh
set -e

# Start Mosquitto in background
mosquitto -c /mosquitto/config/mosquitto.conf &
BROKER_PID=$!

# Wait up to 5 s for the broker to bind port 1883
for i in 1 2 3 4 5 6 7 8 9 10; do
  nc -z localhost 1883 && break
  sleep 0.5
done

# Launch wildcard subscriber that prints every payload
/app/logger.sh &

# If Mosquitto stops, exit the container
wait "$BROKER_PID"
