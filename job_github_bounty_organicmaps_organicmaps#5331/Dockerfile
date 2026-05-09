# syntax=docker/dockerfile:1.4

###############################
# Builder stage (Go)
###############################
FROM golang:1.22-alpine AS builder

# Install build dependencies
RUN apk add --no-cache git ca-certificates

# Set working directory
WORKDIR /src

# Cache go.mod and go.sum
COPY go.mod go.sum ./
RUN go mod download

# Copy source code
COPY . .

# Build the binary
ARG BUILD_FLAGS="-trimpath -ldflags=-s -w"
RUN CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go build $BUILD_FLAGS -o /app/pt-backend ./cmd/pt-backend

###############################
# Runtime stage
###############################
FROM alpine:3.19 AS runtime

# Create non-root user and group
RUN addgroup -S ptgroup && adduser -S -G ptgroup ptuser

# Install runtime dependencies
RUN apk add --no-cache ca-certificates tzdata

# Copy binary from builder
COPY --from=builder /app/pt-backend /usr/local/bin/pt-backend

# Set non-root user
USER ptuser:ptgroup

# Expose service port (adjust if needed)
EXPOSE 8080

# Healthcheck (adjust endpoint if needed)
HEALTHCHECK --interval=30s --timeout=5s --start-period=5s \
  CMD wget -qO- http://localhost:8080/health || exit 1

# Define entrypoint
ENTRYPOINT ["/usr/local/bin/pt-backend"]