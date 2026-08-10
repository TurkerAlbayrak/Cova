FROM alpine:latest

# Install build dependencies (GCC, musl-dev for standard C library, OpenSSL and zlib)
RUN apk add --no-cache gcc musl-dev openssl-dev zlib-dev

# Set the working directory
WORKDIR /app

# Copy the source code into the container
COPY . .

# Compile the framework with OpenSSL and zlib support
# (Using the same compilation flags as build.bat but adapted for Linux)
RUN gcc -Wall -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -DUSE_OPENSSL \
    -Iinclude examples/main.c src/*.c -o server -lssl -lcrypto -lz -lpthread

# Expose the default port
EXPOSE 8080

# Command to run the executable
CMD ["./server"]
