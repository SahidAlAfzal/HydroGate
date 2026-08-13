# -------------------------------------------------------------
# Stage 1: Build Environment
# -------------------------------------------------------------
FROM ubuntu:22.04 AS builder

# Prevent tzdata prompts from blocking the build
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    libssl-dev \
    libhiredis-dev

# Clone and build redis-plus-plus from source 
# (Since it is not always available in standard apt repositories)
WORKDIR /deps
RUN git clone https://github.com/sewenew/redis-plus-plus.git && \
    cd redis-plus-plus && \
    mkdir build && cd build && \
    cmake -DREDIS_PLUS_PLUS_CXX_STANDARD=17 -DREDIS_PLUS_PLUS_USE_TLS=ON .. && \
    make -j$(nproc) && \
    make install

# Now build the HydroGate application
WORKDIR /app
COPY . .

# Compile HydroGate
RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

# -------------------------------------------------------------
# Stage 2: Runtime Environment (Minimal Image)
# -------------------------------------------------------------
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Install only the runtime libraries needed
RUN apt-get update && apt-get install -y \
    libhiredis0.14 \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy the redis++ compiled library from the builder stage
COPY --from=builder /usr/local/lib/libredis++.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/libredis++.a /usr/local/lib/

# Update linker cache so it finds libredis++
RUN ldconfig

# Copy the compiled HydroGate binary
COPY --from=builder /app/build/hydrogate /usr/local/bin/hydrogate

# Set working directory for runtime
WORKDIR /app

# Expose the gateway port
EXPOSE 8080

# Run the proxy
CMD ["hydrogate"]
