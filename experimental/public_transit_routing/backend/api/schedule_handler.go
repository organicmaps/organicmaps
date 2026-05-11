go
package api

import (
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"regexp"
	"strconv"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/golang/protobuf/proto"
	"github.com/rs/zerolog"
	"github.com/rs/zerolog/log"
	"github.com/xeipuuv/gojsonschema"
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
	r.POST("/schedule/:region", handleUploadSchedule(cfg))
	return r
}

// validateRegion ensures the region parameter contains only allowed characters.
func validateRegion(region string) error {
	if region == "" {
		return fmt.Errorf("region is required")
	}
	// Allow alphanumeric, hyphens and underscores.
	matched, _ := regexp.MatchString(`^[a-zA-Z0-9_-]+$`, region)
	if !matched {
		return fmt.Errorf("invalid region '%s': only letters, numbers, hyphens and underscores are allowed", region)
	}
	return nil
}

// JSON schema for schedule upload payloads.
const scheduleUploadSchema = `{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"type": "object",
	"required": ["version", "timestamp", "payload"],
	"properties": {
		"version":   {"type": "integer", "minimum": 0},
		"timestamp": {"type": "integer", "minimum": 0},
		"payload":   {"type": "string", "format": "byte"}
	},
	"additionalProperties": false
}`

// handleUploadSchedule validates the JSON body against a schema and stores the blob.
func handleUploadSchedule(cfg Config) gin.HandlerFunc {
	schemaLoader := gojsonschema.NewStringLoader(scheduleUploadSchema)
	return func(c *gin.Context) {
		region := c.Param("region")
		if err := validateRegion(region); err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
			return
		}

		bodyBytes, err := io.ReadAll(c.Request.Body)
		if err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": "unable to read request body"})
			return
		}
		defer c.Request.Body.Close()

		// Validate JSON against schema.
		documentLoader := gojsonschema.NewBytesLoader(bodyBytes)
		result, err := gojsonschema.Validate(schemaLoader, documentLoader)
		if err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{"error": "schema validation failed"})
			return
		}
		if !result.Valid() {
			var msgs []string
			for _, e := range result.Errors() {
				msgs = append(msgs, e.String())
			}
			c.JSON(http.StatusBadRequest, gin.H{"error": "invalid payload", "details": msgs})
			return
		}

		// Unmarshal into struct.
		var payload struct {
			Version   uint32 `json:"version"`
			Timestamp int64  `json:"timestamp"`
			Payload   string `json:"payload"` // base64-encoded
		}
		if err := json.Unmarshal(bodyBytes, &payload); err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": "invalid JSON format"})
			return
		}

		// Decode base64 payload.
		blobData, err := base64.StdEncoding.DecodeString(payload.Payload)
		if err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": "payload must be base64-encoded"})
			return
		}

		blob := ScheduleBlob{
			Version:   payload.Version,
			Timestamp: payload.Timestamp,
			Payload:   blobData,
		}
		// Marshal protobuf (here we just marshal JSON as placeholder).
		// In real code, use proto.Marshal.
		data, err := json.Marshal(blob)
		if err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to marshal schedule"})
			return
		}

		blobPath := filepath.Join(cfg.BlobDir, region+".blob")
		if err := os.MkdirAll(cfg.BlobDir, 0755); err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to create storage directory"})
			return
		}
		if err := os.WriteFile(blobPath, data, 0644); err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to write schedule blob"})
			return
		}

		c.Status(http.StatusCreated)
	}
}

// handleSchedule serves the latest schedule blob for a given region.
func handleSchedule(cfg Config) gin.HandlerFunc {
	return func(c *gin.Context) {
		region := c.Param("region")
		if err := validateRegion(region); err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
			return
		}

		blobPath := filepath.Join(cfg.BlobDir, region+".blob")
		file, err := os.Open(blobPath)
		if err != nil {
			if os.IsNotExist(err) {
				c.JSON(http.StatusNotFound, gin.H{"error": fmt.Sprintf("schedule not found for region '%s'", region)})
			} else {
				log.Error().Err(err).Str("path", blobPath).Msg("failed to open schedule blob")
				c.JSON(http.StatusInternalServerError, gin.H{"error": "internal server error"})
			}
			return
		}
		defer file.Close()

		data, err := io.ReadAll(file)
		if err != nil {
			log.Error().Err(err).Str("path", blobPath).Msg("failed to read schedule blob")
			c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to read schedule data"})
			return
		}

		var blob ScheduleBlob
		if err := proto.Unmarshal(data, &blob); err != nil {
			log.Error().Err(err).Str("path", blobPath).Msg("invalid protobuf in schedule blob")
			c.JSON(http.StatusInternalServerError, gin.H{"error": "corrupt schedule data"})
			return
		}

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

		if err := validateRegion(region); err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
			return
		}
		if baseVersionStr == "" {
			c.JSON(http.StatusBadRequest, gin.H{"error": "baseVersion is required"})
			return
		}

		baseVersion, err := strconv.ParseUint(baseVersionStr, 10, 32)
		if err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": fmt.Sprintf("invalid baseVersion '%s': must be a non‑negative integer", baseVersionStr)})
			return
		}

		blobPath := filepath.Join(cfg.BlobDir, region+".blob")
		blobFile, err := os.Open(blobPath)
		if err != nil {
			if os.IsNotExist(err) {
				c.JSON(http.StatusNotFound, gin.H{"error": fmt.Sprintf("schedule not found for region '%s'", region)})
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
			c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to read schedule data"})
			return
		}
		var latestBlob ScheduleBlob
		if err := proto.Unmarshal(blobData, &latestBlob); err != nil {
			log.Error().Err(err).Str("path", blobPath).Msg("invalid protobuf in schedule blob")
			c.JSON(http.StatusInternalServerError, gin.H{"error": "corrupt schedule data"})
			return
		}

		if uint64(latestBlob.Version) == baseVersion {
			c.Status(http.StatusNotModified)
			return
		}

		deltaPath := filepath.Join(cfg.DeltaDir, region, strconv.FormatUint(baseVersion, 10)+"->"+strconv.FormatUint(uint64(latestBlob.Version), 10)+".delta")
		deltaFile, err := os.Open(deltaPath)
		if err != nil {
			if os.IsNotExist(err) {
				// No delta available; fallback to full blob.
				c.Redirect(http.StatusFound, "/schedule/"+region)
			} else {
				log.Error().Err(err).Str("path", deltaPath).Msg("failed to open delta file")
				c.JSON(http.StatusInternalServerError, gin.H{"error": "internal server error"})
			}
			return
		}
		defer deltaFile.Close()

		deltaData, err := io.ReadAll(deltaFile)
		if err != nil {
			log.Error().Err(err).Str("path", deltaPath).Msg("failed to read delta file")
			c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to read delta data"})
			return
		}
		var delta DeltaUpdate
		if err := proto.Unmarshal(deltaData, &delta); err != nil {
			log.Error().Err(err).Str("path", deltaPath).Msg("invalid protobuf in delta file")
			c.JSON(http.StatusInternalServerError, gin.H{"error": "corrupt delta data"})
			return
		}

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