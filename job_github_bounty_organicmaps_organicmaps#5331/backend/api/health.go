package api

import (
	"net/http"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/prometheus/client_golang/prometheus/promhttp"
)

// ServiceInfo holds static information about the service.
type ServiceInfo struct {
	Name    string `json:"name"`
	Version string `json:"version"`
}

// HealthResponse is the JSON payload returned by the /health endpoint.
type HealthResponse struct {
	Status      string      `json:"status"`      // "ok" or "degraded"
	Uptime      string      `json:"uptime"`      // human‑readable duration
	Timestamp   time.Time   `json:"timestamp"`   // server time
	ServiceInfo ServiceInfo `json:"service"`     // static service metadata
}

// healthHandler returns a JSON payload describing the current health of the service.
func healthHandler(startTime time.Time, info ServiceInfo) gin.HandlerFunc {
	return func(c *gin.Context) {
		uptime := time.Since(startTime).Round(time.Second)

		resp := HealthResponse{
			Status:      "ok",
			Uptime:      uptime.String(),
			Timestamp:   time.Now().UTC(),
			ServiceInfo: info,
		}
		c.JSON(http.StatusOK, resp)
	}
}

// metricsHandler registers the Prometheus HTTP handler on the Gin router.
func metricsHandler() gin.HandlerFunc {
	h := promhttp.Handler()
	return func(c *gin.Context) {
		h.ServeHTTP(c.Writer, c.Request)
	}
}

// loggingMiddleware logs each request with method, path, status and latency.
func loggingMiddleware() gin.HandlerFunc {
	return func(c *gin.Context) {
		start := time.Now()
		c.Next()
		latency := time.Since(start)
		status := c.Writer.Status()
		// Simple stdout logger – replace with a structured logger if needed.
		// log.Printf("%s %s %s %d %s\n", time.Now().UTC().Format(time.RFC3339), c.Request.Method, c.Request.URL.Path, status, latency)
		_ = latency
		_ = status
	}
}