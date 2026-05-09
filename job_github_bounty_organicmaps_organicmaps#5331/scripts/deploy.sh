#!/usr/bin/env bash
# =============================================================================
# scripts/deploy.sh
# =============================================================================
# Deploy the backend service to production.
#
# Steps performed:
#   1. Build and push Docker image to the container registry.
#   2. Invalidate / update CDN cache for the new image assets.
#   3. Restart the backend service (Docker Swarm / systemd / Kubernetes).
#
# Requirements:
#   - Docker CLI (>= 20.10)
#   - jq (for JSON parsing)
#   - curl (for CDN API calls)
#   - Access to the container registry (Docker Hub, GCR, ECR, etc.)
#   - Access to the service host (SSH key / sudo privileges)
#
# Usage:
#   ./scripts/deploy.sh [options]
#
# Options:
#   -i, --image <name>          Docker image name (default: myorg/backend)
#   -t, --tag   <tag>           Image tag (default: git commit SHA)
#   -r, --registry <url>       Container registry URL (default: docker.io)
#   -c, --cdn-endpoint <url>   CDN API endpoint (required for CDN invalidation)
#   -s, --service <name>        Service name to restart (default: backend)
#   -h, --help                  Show this help message and exit
#
# Example:
#   ./scripts/deploy.sh -i myorg/backend -t v1.2.3 -c https://cdn.example.com/api/purge -s backend
# =============================================================================

set -euo pipefail

# ------------------------------- Logging ------------------------------------
log() {
    local level="$1"
    shift
    local msg="$*"
    local ts
    ts=$(date +"%Y-%m-%d %H:%M:%S")
    echo "[$ts] [$level] $msg"
}

info()   { log "INFO"  "$@"; }
warn()   { log "WARN"  "$@"; }
error()  { log "ERROR" "$@"; }

# ------------------------------- Helpers -----------------------------------
die() {
    error "$@"
    exit 1
}

# Resolve the absolute path of the script directory
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

# ------------------------------- Defaults ----------------------------------
IMAGE_NAME="myorg/backend"
IMAGE_TAG=""
REGISTRY_URL="docker.io"
CDN_ENDPOINT=""
SERVICE_NAME="backend"

# ------------------------------- Argument parsing ---------------------------
while [[ $# -gt 0 ]]; do
    case "$1" in
        -i|--image)
            IMAGE_NAME="${2:?Missing argument for $1}"
            shift 2
            ;;
        -t|--tag)
            IMAGE_TAG="${2:?Missing argument for $1}"
            shift 2
            ;;
        -r|--registry)
            REGISTRY_URL="${2:?Missing argument for $1}"
            shift 2
            ;;
        -c|--cdn-endpoint)
            CDN_ENDPOINT="${2:?Missing argument for $1}"
            shift 2
            ;;
        -s|--service)
            SERVICE_NAME="${2:?Missing argument for $1}"
            shift 2
            ;;
        -h|--help)
            grep '^#' "$0" | sed 's/^# //;1,2d;$d'
            exit 0
            ;;
        *)
            die "Unknown option: $1"
            ;;
    esac
done

# ------------------------------- Validation --------------------------------
[[ -n "$CDN_ENDPOINT" ]] || die "CDN endpoint is required. Use -c/--cdn-endpoint."
[[ -n "$IMAGE_TAG" ]] || {
    # Use the short git commit SHA as default tag
    if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        IMAGE_TAG=$(git rev-parse --short HEAD)
        info "Detected git commit SHA as tag: $IMAGE_TAG"
    else
        die "Cannot determine image tag. Provide one via -t/--tag."
    fi
}

FULL_IMAGE="${REGISTRY_URL}/${IMAGE_NAME}:${IMAGE_TAG}"

# ------------------------------- Functions ---------------------------------
push_image() {
    info "Building Docker image $FULL_IMAGE"
    docker build -t "$FULL_IMAGE" .

    info "Pushing Docker image to $REGISTRY_URL"
    docker push "$FULL_IMAGE"
}

invalidate_cdn() {
    info "Invalidating CDN cache for $FULL_IMAGE"
    # Example payload – adapt to your CDN provider
    payload=$(jq -n --arg img "$FULL_IMAGE" '{paths: [$img]}')
    response=$(curl -s -X POST "$CDN_ENDPOINT" \
        -H "Content-Type: application/json" \
        -d "$payload" \
        -w "%{http_code}")

    http_code="${response: -3}"
    body="${response%???}"
    if [[ "$http_code" -ge 200 && "$http_code" -lt 300 ]]; then
        info "CDN invalidation succeeded."
    else
        die "CDN invalidation failed (HTTP $http_code): $body"
    fi
}

restart_service() {
    info "Restarting service $SERVICE_NAME"
    # Detect the orchestration method
    if command -v docker-compose >/dev/null 2>&1; then
        info "Using docker-compose"
        docker-compose pull "$SERVICE_NAME"
        docker-compose up -d --no-deps "$SERVICE_NAME"
    elif command -v kubectl >/dev/null 2>&1; then
        info "Using kubectl"
        kubectl set image deployment/"$SERVICE_NAME" "$SERVICE_NAME"="$FULL_IMAGE"
        kubectl rollout restart deployment/"$SERVICE_NAME"
    elif command -v systemctl >/dev/null 2>&1; then
        info "Using systemd"
        sudo systemctl restart "$SERVICE_NAME"
    else
        die "No known service manager found (docker-compose, kubectl, systemd)."
    fi
}

# ------------------------------- Main --------------------------------------
main() {
    info "=== Deployment started ==="
    info "Image: $FULL_IMAGE"
    info "CDN endpoint: $CDN_ENDPOINT"
    info "Service: $SERVICE_NAME"

    push_image
    invalidate_cdn
    restart_service

    info "=== Deployment completed successfully ==="
}

# Run the main function and trap any unexpected error
trap 'error "Unexpected error on line $LINENO. Exiting."; exit 1' ERR
main