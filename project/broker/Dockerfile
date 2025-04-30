# ---- Lightweight MQTT broker with built-in “echo” subscriber ----
FROM eclipse-mosquitto:2.0

WORKDIR /app                    # keep helper scripts together

# ---------- broker configuration ----------
COPY mosquitto.conf /mosquitto/config/mosquitto.conf

# ---------- helper scripts ----------
COPY logger.sh      /app/logger.sh
COPY entrypoint.sh  /app/entrypoint.sh
RUN chmod +x /app/*.sh          # make both scripts executable

# ---------- runtime ----------
EXPOSE 1883
ENTRYPOINT ["/app/entrypoint.sh"]
        