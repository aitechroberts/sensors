# Lightweight MQTT broker with built‑in message “echo”
FROM eclipse-mosquitto:2.0

# broker config
COPY mosquitto.conf /mosquitto/config/mosquitto.conf

# logs subscriber to every topic and prints payloads
COPY logger.sh /logger.sh
RUN chmod +x /logger.sh

# two long‑running processes: broker + stdout logger
CMD sh -c "mosquitto -c /mosquitto/config/mosquitto.conf & \
            sleep 2 && /logger.sh"
