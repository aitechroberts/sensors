# Start from an official Ubuntu 22.04 base
FROM ubuntu:22.04

# Install build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Create a working directory
WORKDIR /app

# Copy source code into the container
COPY vehicle_api vehicle_api

# Make sure the binary is executable
RUN chmod +x vehicle_api

# Expose port 9090 for inbound connections
EXPOSE 9090

# Specify the entrypoint to run the server
ENTRYPOINT ["./vehicle_api"]
