package api

import (
	"encoding/json"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/golang/protobuf/proto"
	"github.com/rs/zerolog"
	"github.com/rs/zerolog/log"
)

// ScheduleBlob represents the compact binary schedule format.
// In a real implementation this would be generated from a .proto file.
type ScheduleBlob struct {
	// Version of the schedule format.
	Version uint32 `json:"version"`
	// Timestamp of the latest update (unix seconds).
	Timestamp int64 `json:"timestamp"`
	// Payload contains the binary protobuf data.
	Payload []byte `json:"payload"`
}

// DeltaUpdate represents a delta between two schedule versions.
type DeltaUpdate struct {
	// BaseVersion is the version the delta applies to.
	BaseVersion uint32 `json:"base_version"`
	// NewVersion is the version after applying the delta.
	NewVersion uint32 `json:"new_version"`
	// Payload contains the protobuf delta.
	Payload []byte `json:"payload"`
}

// Config holds runtime configuration for the handler.
type Config struct {
	// Directory where schedule blobs are stored.
	BlobDir string
	// Directory where delta files are stored.
	DeltaDir string
	// MaxAge for HTTP cache headers.
	MaxAge time.Duration
}

// NewRouter creates a Gin router with the schedule endpoints registered.
func NewRouter(cfg Config) *gin.Engine {
	r := gin.New()
	r.Use(gin.Recovery())
	r.Use(gin.LoggerWithWriter(log.Logger.Output()))
	r.GET("/schedule/:region", handleSchedule(cfg))
	r.GET("/schedule/:region/delta/:baseVersion", handleDelta(cfg))
	return r
}

// handleSchedule serves the latest schedule blob for a given region.
func handleSchedule(cfg Config) gin.HandlerFunc {
	return func(c *gin.Context) {
		region := c.Param("region")
		if region == "" {
			c.JSON(http.StatusBadRequest, gin.H{"error": "region is required"})
			return
		}

		blobPath := filepath.Join(cfg.BlobDir, region+".blob")
		file, err := os.Open(blobPath)
		if err != nil {
			if os.IsNotExist(err) {
				c.JSON(http.StatusNotFound, gin.H{"error": "schedule not found"})
			} else {
				log.Error().Err(err).Str("path", blobPath).Msg("failed to open schedule blob")
				c.JSON(http.StatusInternalServerError, gin.H{"error": "internal server error"})
			}
			return
		}
		defer file.Close()

		// Read the whole file (expected to be small enough for a single read)
		data, err := io.ReadAll(file)
		if err != nil {
			log.Error().Err(err).Str("path", blobPath).Msg("failed to read schedule blob")
			c.JSON(http.StatusInternalServerError, gin.H{"error": "internal server error"})
			return
		}

		// Unmarshal to verify protobuf integrity (optional, can be disabled for perf)
		var blob ScheduleBlob
		if err := proto.Unmarshal(data, &blob); err != nil {
			log.Error().Err(err).Str("path", blobPath).Msg("invalid protobuf in schedule blob")
			c.JSON(http.StatusInternalServerError, gin.H{"error": "corrupt schedule data"})
			return
		}

		// Set cache headers
		c.Header("Cache-Control", "public, max-age="+strconv.Itoa(int(cfg.MaxAge.Seconds())))
		c.Header("Content-Type", "application/octet-stream")
		c.Header("ETag", strconv.FormatUint(uint64(blob.Version), 10))

		c.Data(http.StatusOK, "application/octet-stream", blob.Payload)
	}
}

// handleDelta serves a delta update from a base version to the latest version.
func handleDelta(cfg Config) gin.HandlerFunc {
	return func(c *gin.Context) {
		region := c.Param("region")
		baseVersionStr := c.Param("baseVersion")
		if region == "" || baseVersionStr == "" {
			c.JSON(http.StatusBadRequest, gin.H{"error": "region and baseVersion are required"})
			return
		}
		baseVersion, err := strconv.ParseUint(baseVersionStr, 10, 32)
		if err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": "invalid baseVersion"})
			return
		}

		// Find the latest version for the region.
		blobPath := filepath.Join(cfg.BlobDir, region+".blob")
		blobFile, err := os.Open(blobPath)
		if err != nil {
			if os.IsNotExist(err) {
				c.JSON(http.StatusNotFound, gin.H{"error": "schedule not found"})
			} else {
				log.Error().Err(err).Str("path", blobPath).Msg("failed to open schedule blob")
				c.JSON(http.StatusInternalServerError, gin.H{"error": "internal server error"})
			}
			return
		}
		defer blobFile.Close()

		blobData, err := io.ReadAll(blobFile)
		if err != nil {
			log.Error().Err(err).Str("path", blobPath).Msg("failed to read schedule blob")
			c.JSON(http.StatusInternalServerError, gin.H{"error": "internal server error"})
			return
		}
		var latestBlob ScheduleBlob
		if err := proto.Unmarshal(blobData, &latestBlob); err != nil {
			log.Error().Err(err).Str("path", blobPath).Msg("invalid protobuf in schedule blob")
			c.JSON(http.StatusInternalServerError, gin.H{"error": "corrupt schedule data"})
			return
		}

		// If client already has the latest version, respond with 304.
		if uint64(latestBlob.Version) == baseVersion {
			c.Status(http.StatusNotModified)
			return
		}

		// Load delta file.
		deltaPath := filepath.Join(cfg.DeltaDir, region, strconv.FormatUint(baseVersion, 10)+"->"+strconv.FormatUint(uint64(latestBlob.Version), 10)+".delta")
		deltaFile, err := os.Open(deltaPath)
		if err != nil {
			if os.IsNotExist(err) {
				// No delta available, fallback to full blob.
				c.Redirect(http.StatusFound, "/schedule/"+region)
				return
			}
			log.Error().Err(err).Str("path", deltaPath).Msg("failed to open delta file")
			c.JSON(http.StatusInternalServerError, gin.H{"error": "internal server error"})
			return
		}
		defer deltaFile.Close()

		deltaData, err := io.ReadAll(deltaFile)
		if err != nil {
			log.Error().Err(err).Str("path", deltaPath).Msg("failed to read delta file")
			c.JSON(http.StatusInternalServerError, gin.H{"error": "internal server error"})
			return
		}
		var delta DeltaUpdate
		if err := proto.Unmarshal(deltaData, &delta); err != nil {
			log.Error().Err(err).Str("path", deltaPath).Msg("invalid protobuf in delta file")
			c.JSON(http.StatusInternalServerError, gin.H{"error": "corrupt delta data"})
			return
		}

		// Set cache headers for delta (shorter TTL).
		c.Header("Cache-Control", "public, max-age="+strconv.Itoa(int(cfg.MaxAge.Seconds()/2)))
		c.Header("Content-Type", "application/octet-stream")
		c.Header("ETag", strconv.FormatUint(uint64(delta.NewVersion), 10))

		c.Data(http.StatusOK, "application/octet-stream", delta.Payload)
	}
}

// LoadConfig reads configuration from environment variables.
// It provides sensible defaults for development.
func LoadConfig() Config {
	blobDir := os.Getenv("SCHEDULE_BLOB_DIR")
	if blobDir == "" {
		blobDir = "./data/blobs"
	}
	deltaDir := os.Getenv("SCHEDULE_DELTA_DIR")
	if deltaDir == "" {
		deltaDir = "./data/deltas"
	}
	maxAgeSec := 3600 // default 1 hour
	if v := os.Getenv("SCHEDULE_MAX_AGE"); v != "" {
		if secs, err := strconv.Atoi(v); err == nil && secs > 0 {
			maxAgeSec = secs
		}
	}
	return Config{
		BlobDir: blobDir,
		DeltaDir: deltaDir,
		MaxAge:   time.Duration(maxAgeSec) * time.Second,
	}
}

// main entry point for the schedule service.
// It sets up logging, loads configuration and starts the HTTP server.
func main() {
	// Initialize zerolog with pretty console output for dev.
	log.Logger = log.Output(zerolog.ConsoleWriter{Out: os.Stderr})

	cfg := LoadConfig()
	r := NewRouter(cfg)

	// Use PORT env var or default to 8080.
	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}
	addr := ":" + port

	log.Info().Str("addr", addr).Msg("starting schedule service")
	if err := r.Run(addr); err != nil {
		log.Fatal().Err(err).Msg("failed to start server")
	}
}

// ---------------------------------------------------------------------------
// Helper functions for testing and maintenance.
// ---------------------------------------------------------------------------

// SaveBlob writes a ScheduleBlob to the appropriate file.
// Used by data ingestion pipelines.
func SaveBlob(cfg Config, region string, blob *ScheduleBlob) error {
	data, err := proto.Marshal(blob)
	if err != nil {
		return err
	}
	path := filepath.Join(cfg.BlobDir, region+".blob")
	if err := os.MkdirAll(filepath.Dir(path), 0755); err != nil {
		return err
	}
	return os.WriteFile(path, data, 0644)
}

// SaveDelta writes a DeltaUpdate to the appropriate file.
// Used by data ingestion pipelines.
func SaveDelta(cfg Config, region string, delta *DeltaUpdate) error {
	data, err := proto.Marshal(delta)
	if err != nil {
		return err
	}
	dir := filepath.Join(cfg.DeltaDir, region)
	if err := os.MkdirAll(dir, 0755); err != nil {
		return err
	}
	filename := strconv.FormatUint(uint64(delta.BaseVersion), 10) + "->" + strconv.FormatUint(uint64(delta.NewVersion), 10) + ".delta"
	path := filepath.Join(dir, filename)
	return os.WriteFile(path, data, 0644)
}

// ListRegions returns a slice of region identifiers for which schedule blobs exist.
// Useful for health checks and monitoring.
func ListRegions(cfg Config) ([]string, error) {
	entries, err := os.ReadDir(cfg.BlobDir)
	if err != nil {
		return nil, err
	}
	var regions []string
	for _, entry := range entries {
		if entry.IsDir() {
			continue
		}
		name := entry.Name()
		if ext := filepath.Ext(name); ext == ".blob" {
			regions = append(regions, name[:len(name)-len(ext)])
		}
	}
	return regions, nil}

// ---------------------------------------------------------------------------
// JSON representation for debugging and health endpoints.
// ---------------------------------------------------------------------------

// HealthResponse aggregates basic health information.
type HealthResponse struct {
	Status    string   `json:"status"`
	Regions   []string `json:"regions,omitempty"`
	Timestamp int64    `json:"timestamp"`
}

// HealthHandler returns a JSON health payload.
func HealthHandler(cfg Config) gin.HandlerFunc {
	return func(c *gin.Context) {
		regions, err := ListRegions(cfg)
		if err != nil {
			log.Error().Err(err).Msg("failed to list regions")
			c.JSON(http.StatusInternalServerError, gin.H{"error": "cannot enumerate regions"})
			return
		}
		resp := HealthResponse{
			Status:    "ok",
			Regions:   regions,
			Timestamp: time.Now().Unix(),
		}
		c.JSON(http.StatusOK, resp)
	}
}

// RegisterHealth registers the /health endpoint on the given router.
func RegisterHealth(r *gin.Engine, cfg Config) {
	r.GET("/health", HealthHandler(cfg))
}