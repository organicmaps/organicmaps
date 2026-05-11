dockerfile
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
RUN CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go build $BUILD_FLAGS -o /pt-backend ./cmd/pt-backend

# Remove build caches to keep image small
RUN rm -rf /root/.cache/go-build

###############################
# Runtime stage
###############################
FROM alpine:3.19 AS runtime

# Install runtime dependencies
RUN apk add --no-cache ca-certificates tzdata curl

# Create non-root user and group
RUN addgroup -S ptgroup && adduser -S -G ptgroup ptuser

# Copy binary from builder
COPY --from=builder /pt-backend /usr/local/bin/pt-backend

# Use non-root user
USER ptuser:ptgroup

# Expose service port
EXPOSE 8080

# Healthcheck
HEALTHCHECK --interval=30s --timeout=5s --start-period=5s \
  CMD curl -f http://localhost:8080/health || exit 1

ENTRYPOINT ["/usr/local/bin/pt-backend"]