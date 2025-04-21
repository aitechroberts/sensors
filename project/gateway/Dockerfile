# Use a slim Python base image for smaller size
FROM python:3.10-slim

# Create working directory
WORKDIR /app

# Copy requirements.txt and install (even though it's empty or minimal)
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
RUN pip install --no-cache-dir paho-mqtt

# Copy the gateway script into the container
COPY gateway.py .

# Run the gateway script
CMD ["python", "gateway.py"]
