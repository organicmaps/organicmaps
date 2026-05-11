go
package api

import (
	"log"
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
	Status       string            `json:"status"`       // "ok", "degraded", "unhealthy"
	Version      string            `json:"version"`      // service version
	Dependencies map[string]string `json:"dependencies"` // downstream health statuses
}

// healthHandler returns a JSON payload describing the current health of the service.
func healthHandler(startTime time.Time, info ServiceInfo) gin.HandlerFunc {
	return func(c *gin.Context) {
		depStatus := map[string]string{
			"database": "unknown",
			"storage":  "unknown",
		}
		overallStatus := "ok"

		// Check database
		if err := checkDatabase(); err != nil {
			log.Printf("Database health check failed: %v", err)
			depStatus["database"] = "unhealthy"
			overallStatus = "degraded"
		} else {
			depStatus["database"] = "healthy"
		}

		// Check storage
		if err := checkStorage(); err != nil {
			log.Printf("Storage health check failed: %v", err)
			depStatus["storage"] = "unhealthy"
			if overallStatus == "ok" {
				overallStatus = "degraded"
			}
		} else {
			depStatus["storage"] = "healthy"
		}

		// If any dependency is unhealthy, mark overall as unhealthy
		for _, s := range depStatus {
			if s == "unhealthy" {
				overallStatus = "unhealthy"
				break
			}
		}

		resp := HealthResponse{
			Status:       overallStatus,
			Version:      info.Version,
			Dependencies: depStatus,
		}
		c.JSON(http.StatusOK, resp)
	}
}

// Placeholder for a real database health check.
// Replace with actual DB ping/connection logic.
func checkDatabase() error {
	// Example: return nil if DB is reachable, otherwise return an error.
	// err := db.Ping()
	// return err
	return nil // Assume healthy for now
}

// Placeholder for a real storage health check.
// Replace with actual storage service ping/connection logic.
func checkStorage() error {
	// Example: return nil if storage is reachable, otherwise return an error.
	// err := storageClient.Ping()
	// return err
	return nil // Assume healthy for now
}

// metricsHandler registers the Prometheus HTTP handler on the Gin router.
func metricsHandler() gin.HandlerFunc {
	h := promhttp.Handler()
	return func(c *gin.Context) {
		h.ServeHTTP(c.Writer, c.Request)
	}
}

// loggingMiddleware logs each request with method, path, status and latency.
// It also logs failures (status >= 500) with error details.
func loggingMiddleware() gin.HandlerFunc {
	return func(c *gin.Context) {
		start := time.Now()
		c.Next()
		latency := time.Since(start)
		status := c.Writer.Status()
		if status >= 500 {
			log.Printf("[ERROR] %s %s %s %d %s", time.Now().UTC().Format(time.RFC3339), c.Request.Method, c.Request.URL.Path, status, latency)
		} else {
			log.Printf("[INFO] %s %s %s %d %s", time.Now().UTC().Format(time.RFC3339), c.Request.Method, c.Request.URL.Path, status, latency)
		}
	}
}