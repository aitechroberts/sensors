# Use a slim Python base image for smaller size
FROM python:3.11-slim

# Create working directory
WORKDIR /app

# Copy files
COPY gateway.py .
COPY supervisord.conf /etc/supervisor/conf.d/


# Copy requirements.txt and install (even though it's empty or minimal)
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
RUN pip install --no-cache-dir paho-mqtt mqtt-exporter==1.7.1 supervisor

# Copy the gateway script into the container

# Run the gateway script
CMD ["supervisord", "-n", "-c", "/etc/supervisor/conf.d/supervisord.conf"]  # Supervisor launches both gateway & exporter
