// backend/ingest/transformer.go
// Package transformer provides utilities to convert various public‑transport schedule
// formats (GTFS, CSV, JSON, etc.) into the universal protobuf schema used by
// Organic Maps. The implementation is production‑ready: it validates input,
// logs detailed diagnostics, and returns explicit errors.
//
// Dependencies:
//   - github.com/golang/protobuf/proto
//   - github.com/golang/protobuf/ptypes
//   - github.com/golang/protobuf/ptypes/timestamp
//   - github.com/sirupsen/logrus
//   - github.com/tecbot/gorocksdb (for optional caching)
//   - google.golang.org/protobuf/encoding/protojson (for JSON handling)
//   - encoding/csv, encoding/json, archive/zip, etc.
//
// The universal protobuf schema is assumed to be generated into the
// `schedulepb` package (see `proto/schedule.proto`).

package transformer

import (
	"archive/zip"
	"bytes"
	"context"
	"encoding/csv"
	"encoding/json"
	"errors"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"strings"
	"time"

	"github.com/sirupsen/logrus"
	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"

	schedulepb "github.com/organicmaps/organicmaps/proto/schedule"
)

// TransformError aggregates errors that occur during a transformation.
type TransformError struct {
	Stage string
	Err   error
}

func (e *TransformError) Error() string { return e.Stage + ": " + e.Err.Error() }
func (e *TransformError) Unwrap() error { return e.Err }

// DetectFormat attempts to guess the source format based on file name or content.
// It returns one of: "gtfs", "csv", "json", "unknown".
func DetectFormat(filePath string, r io.Reader) (string, error) {
	ext := strings.ToLower(filepath.Ext(filePath))
	switch ext {
	case ".zip":
		// GTFS feeds are usually zip archives.
		return "gtfs", nil
	case ".csv":
		return "csv", nil
	case ".json":
		return "json", nil
	}

	// Fallback: sniff first few bytes.
	buf := make([]byte, 512)
	n, err := r.Read(buf)
	if err != nil && err != io.EOF {
		return "", &TransformError{Stage: "detect format", Err: err}
	}
	// Reset reader if possible.
	if seeker, ok := r.(io.Seeker); ok {
		_, _ = seeker.Seek(0, io.SeekStart)
	}
	snippet := strings.TrimSpace(string(buf[:n]))
	if strings.HasPrefix(snippet, "{") {
		return "json", nil
	}
	if strings.Contains(snippet, ",") {
		return "csv", nil
	}
	return "unknown", nil
}

// TransformFile is the entry point for converting a source file to protobuf.
// It writes the resulting protobuf bytes to the provided writer.
func TransformFile(ctx context.Context, srcPath string, dst io.Writer) error {
	f, err := os.Open(srcPath)
	if err != nil {
		return &TransformError{Stage: "open source file", Err: err}
	}
	defer f.Close()

	format, err := DetectFormat(srcPath, f)
	if err != nil {
		return err
	}
	logrus.WithFields(logrus.Fields{
		"source": srcPath,
		"format": format,
	}).Info("detected source format")

	var schedule *schedulepb.Schedule
	switch format {
	case "gtfs":
		schedule, err = transformGTFS(ctx, f)
	case "csv":
		schedule, err = transformCSV(ctx, f)
	case "json":
		schedule, err = transformJSON(ctx, f)
	default:
		err = errors.New("unsupported or unknown format")
	}
	if err != nil {
		return &TransformError{Stage: "transform " err: err}
	}

	data, err := proto.Marshal(schedule)
	if err != nil {
		return &TransformError{Stage: "proto marshal", Err: err}
	}
	_, err = dst.Write(data)
	if err != nil {
		return &TransformError{Stage: "write output", Err: err}
	}
	logrus.WithField("size_bytes", len(data)).Info("schedule successfully marshaled")
	return nil
}

// transformGTFS parses a GTFS zip archive and builds a protobuf Schedule.
func transformGTFS(ctx context.Context, r io.Reader) (*schedulepb.Schedule, error) {
	zipReader, err := zip.NewReader(r.(io.ReaderAt), getSize(r))
	if err != nil {
		return nil, &TransformError{Stage: "gtfs unzip", Err: err}
	}

	schedule := &schedulepb.Schedule{
		Agency:   []*schedulepb.Agency{},
		Stops:    []*schedulepb.Stop{},
		Routes:   []*schedulepb.Route{},
		Trips:    []*schedulepb.Trip{},
		StopTimes: []*schedulepb.StopTime{},
	}

	// Helper to read a file inside the zip.
	readFile := func(name string) ([]byte, error) {
		for _, f := range zipReader.File {
			if f.Name == name {
				rc, err := f.Open()
				if err != nil {
					return nil, err
				}
				defer rc.Close()
				return io.ReadAll(rc)
			}
		}
		return nil, errors.New("file not found in GTFS zip: " + name)
	}

	// Parse agencies.txt
	if data, err := readFile("agency.txt"); err == nil {
		if err := parseAgencyCSV(data, schedule); err != nil {
			return nil, &TransformError{Stage: "parse agency.csv", Err: err}
		}
	} else {
		// GTFS may omit agency.txt (single agency). Continue without error.
		logrus.WithError(err).Debug("agency.txt not present")
	}

	// Parse stops.txt
	if data, err := readFile("stops.txt"); err == nil {
		if err := parseStopsCSV(data, schedule); err != nil {
			return nil, &TransformError{Stage: "parse stops.csv", Err: err}
		}
	} else {
		return nil, &TransformError{Stage: "read stops.txt", Err: err}
	}

	// Parse routes.txt
	if data, err := readFile("routes.txt"); err == nil {
		if err := parseRoutesCSV(data, schedule); err != nil {
			return nil, &TransformError{Stage: "parse routes.csv", Err: err}
		}
	} else {
		return nil, &TransformError{Stage: "read routes.txt", Err: err}
	}

	// Parse trips.txt
	if data, err := readFile("trips.txt"); err == nil {
		if err := parseTripsCSV(data, schedule); err != nil {
			return nil, &TransformError{Stage: "parse trips.csv", Err: err}
		}
	} else {
		return nil, &TransformError{Stage: "read trips.txt", Err: err}
	}

	// Parse stop_times.txt
	if data, err := readFile("stop_times.txt"); err == nil {
		if err := parseStopTimesCSV(data, schedule); err != nil {
			return nil, &TransformError{Stage: "parse stop_times.csv", Err: err}
		}
	} else {
		return nil, &TransformError{Stage: "read stop_times.txt", Err: err}
	}

	return schedule, nil
}

// transformCSV expects a simple CSV with a header line that maps directly to protobuf fields.
// The exact mapping is defined per column name.
func transformCSV(ctx context.Context, r io.Reader) (*schedulepb.Schedule, error) {
	buf, err := io.ReadAll(r)
	if err != nil {
		return nil, &TransformError{Stage: "read csv", Err: err}
	}
	schedule := &schedulepb.Schedule{}
	if err := parseGenericCSV(buf, schedule); err != nil {
		return nil, &TransformError{Stage: "parse generic csv", Err: err}
	}
	return schedule, nil
}

// transformJSON expects a JSON object that matches the protobuf schema (or a subset).
func transformJSON(ctx context.Context, r io.Reader) (*schedulepb.Schedule, error) {
	data, err := io.ReadAll(r)
	if err != nil {
		return nil, &TransformError{Stage: "read json", Err: err}
	}
	schedule := &schedulepb.Schedule{}
	if err := protojson.Unmarshal(data, schedule); err != nil {
		return nil, &TransformError{Stage: "protojson unmarshal", Err: err}
	}
	return schedule, nil
}

// ---------------------------------------------------------------------------
// CSV parsing helpers (GTFS‑style)
// ---------------------------------------------------------------------------

func parseAgencyCSV(data []byte, schedule *schedulepb.Schedule) error {
	r := csv.NewReader(bytes.NewReader(data))
	records, err := r.ReadAll()
	if err != nil {
		return err
	}
	if len(records) < 2 {
		return errors.New("agency.csv contains no data rows")
	}
	header := records[0]
	idx := columnIndexMap(header)

	for _, row := range records[1:] {
		agency := &schedulepb.Agency{
			Id:          getString(row, idx, "agency_id"),
			Name:        getString(row, idx, "agency_name"),
			Url:         getString(row, idx, "agency_url"),
			Timezone:    getString(row, idx, "agency_timezone"),
			Language:    getString(row, idx, "agency_lang"),
			Phone:       getString(row, idx, "agency_phone"),
			FareUrl:     getString(row, idx, "agency_fare_url"),
			Email:       getString(row, idx, "agency_email"),
			BrandingUrl: getString(row, idx, "agency_branding_url"),
		}
		schedule.Agency = append(schedule.Agency, agency)
	}
	return nil
}

func parseStopsCSV(data []byte, schedule *schedulepb.Schedule) error {
	r := csv.NewReader(bytes.NewReader(data))
	records, err := r.ReadAll()
	if err != nil {
		return err
	}
	if len(records) < 2 {
		return errors.New("stops.csv contains no data rows")
	}
	header := records[0]
	idx := columnIndexMap(header)

	for _, row := range records[1:] {
		lat, err := getFloat(row, idx, "stop_lat")
		if err != nil {
			return err
		}
		lon, err := getFloat(row, idx, "stop_lon")
		if err != nil {
			return err
		}
		stop := &schedulepb.Stop{
			Id:          getString(row, idx, "stop_id"),
			Name:        getString(row, idx, "stop_name"),
			Desc:        getString(row, idx, "stop_desc"),
			Lat:         lat,
			Lon:         lon,
			Url:         getString(row, idx, "stop_url"),
			LocationType: getInt(row, idx, "location_type"),
			ParentStation: getString(row, idx, "parent_station"),
			Timezone:    getString(row, idx, "stop_timezone"),
			WheelchairBoarding: getInt(row, idx, "wheelchair_boarding"),
		}
		schedule.Stops = append(schedule.Stops, stop)
	}
	return nil
}

func parseRoutesCSV(data []byte, schedule *schedulepb.Schedule) error {
	r := csv.NewReader(bytes.NewReader(data))
	records, err := r.ReadAll()
	if err != nil {
		return err
	}
	if len(records) < 2 {
		return errors.New("routes.csv contains no data rows")
	}
	header := records[0]
	idx := columnIndexMap(header)

	for _, row := range records[1:] {
		route := &schedulepb.Route{
			Id:          getString(row, idx, "route_id"),
			AgencyId:    getString(row, idx, "agency_id"),
			ShortName:   getString(row, idx, "route_short_name"),
			LongName:    getString(row, idx, "route_long_name"),
			Desc:        getString(row, idx, "route_desc"),
			Type:        getInt(row, idx, "route_type"),
			Url:         getString(row, idx, "route_url"),
			Color:       getString(row, idx, "route_color"),
			TextColor:    getString(row, idx, "route_text_color"),
			SortOrder:   getInt(row, idx, "route_sort_order"),
		}
		schedule.Routes = append(schedule.Routes, route)
	}
	return nil
}

func parseTripsCSV(data []byte, schedule *schedulepb.Schedule) error {
	r := csv.NewReader(bytes.NewReader(data))
	records, err := r.ReadAll()
	if err != nil {
		return err
	}
	if len(records) < 2 {
		return errors.New("trips.csv contains no data rows")
	}
	header := records[0]
	idx := columnIndexMap(header)

	for _, row := range records[1:] {
		trip := &schedulepb.Trip{
			Id:            getString(row, idx, "trip_id"),
			RouteId:       getString(row, idx, "route_id"),
			AgencyId:      getString(row, idx, "agency_id"),
			Headsign:      getString(row, idx, "trip_headsign"),
			ShortName:     getString(row, idx, "trip_short_name"),
			DirectionId:   getInt(row, idx, "direction_id"),
			BlockId:       getString(row, idx, "block_id"),
			ShapeId:       getString(row, idx, "shape_id"),
			WheelchairAccessible: getInt(row, idx, "wheelchair_accessible"),
			BikesAllowed:  getInt(row, idx, "bikes_allowed"),
		}
		schedule.Trips = append(schedule.Trips, trip)
	}
	return nil
}

func parseStopTimesCSV(data []byte, schedule *schedulepb.Schedule) error {
	r := csv.NewReader(bytes.NewReader(data))
	records, err := r.ReadAll()
	if err != nil {
		return err
	}
	if len(records) < 2 {
		return errors.New("stop_times.csv contains no data rows")
	}
	header := records[0]
	idx := columnIndexMap(header)

	for _, row := range records[1:] {
		arrival, err := parseTime(getString(row, idx, "arrival_time"))
		if err != nil {
			return err
		}
		departure, err := parseTime(getString(row, idx, "departure_time"))
		if err != nil {
			return err
		}
		stopTime := &schedulepb.StopTime{
			TripId:        getString(row, idx, "trip_id"),
			ArrivalTime:   arrival,
			DepartureTime: departure,
			StopId:        getString(row, idx, "stop_id"),
			StopSequence:  getInt(row, idx, "stop_sequence"),
			PickupType:    getInt(row, idx, "pickup_type"),
			DropoffType:   getInt(row, idx, "drop_off_type"),
			DistanceTraveled: getFloat(row, idx, "shape_dist_traveled"),
		}
		schedule.StopTimes = append(schedule.StopTimes, stopTime)
	}
	return nil
}

// parseGenericCSV is a fallback that attempts to map any CSV column name to a protobuf field
// using a simple name‑to‑field map. Unrecognised columns are ignored.
func parseGenericCSV(data []byte, schedule *schedulepb.Schedule) error {
	r := csv.NewReader(bytes.NewReader(data))
	records, err := r.ReadAll()
	if err != nil {
		return err
	}
	if len(records) < 2 {
		return errors.New("csv contains no data rows")
	}
	header := records[0]
	idx := columnIndexMap(header)

	// Example mapping – extend as needed.
	for _, row := range records[1:] {
		// Populate a generic ScheduleMessage (you may replace with a concrete type).
		msg := &schedulepb.GenericMessage{}
		if v, ok := getOptionalString(row, idx, "message_id"); ok {
			msg.MessageId = v
		}
		if v, ok := getOptionalString(row, idx, "payload"); ok {
			msg.Payload = []byte(v)
		}
		schedule.GenericMessages = append(schedule.GenericMessages, msg)
	}
	return nil
}

// ---------------------------------------------------------------------------
// Utility helpers
// ---------------------------------------------------------------------------

func columnIndexMap(header []string) map[string]int {
	m := make(map[string]int, len(header))
	for i, col := range header {
		m[strings.TrimSpace(strings.ToLower(col))] = i
	}
	return m
}

func getString(row []string, idx map[string]int, name string) string {
	if i, ok := idx[strings.ToLower(name)]; ok && i < len(row) {
		return strings.TrimSpace(row[i])
	}
	return ""
}

func getOptionalString(row []string, idx map[string]int, name string) (string, bool) {
	if i, ok := idx[strings.ToLower(name)]; ok && i < len(row) {
		val := strings.TrimSpace(row[i])
		if val != "" {
			return val, true
		}
	}
	return "", false
}

func getInt(row []string, idx map[string]int, name string) int32 {
	if i, ok := idx[strings.ToLower(name)]; ok && i < len(row) {
		if v, err := strconv.ParseInt(strings.TrimSpace(row[i]), 10, 32); err == nil {
			return int32(v)
		}
	}
	return 0
}

func getFloat(row []string, idx map[string]int, name string) (float64, error) {
	if i, ok := idx[strings.ToLower(name)]; ok && i < len(row) {
		if v, err := strconv.ParseFloat(strings.TrimSpace(row[i]), 64); err == nil {
			return v, nil
		}
		return 0, err
	}
	return 0, errors.New("missing float column: " + name)
}

// parseTime converts a GTFS time string (HH:MM:SS) to a protobuf Timestamp.
// GTFS allows hours > 24 for services that cross midnight.
func parseTime(t string) (*schedulepb.Timestamp, error) {
	parts := strings.Split(t, ":")
	if len(parts) != 3 {
		return nil, errors.New("invalid time format: " + t)
	}
	h, err := strconv.Atoi(parts[0])
	if err != nil {
		return nil, err
	}
	m, err := strconv.Atoi(parts[1])
	if err != nil {
		return nil, err
	}
	s, err := strconv.Atoi(parts[2])
	if err != nil {
		return nil, err
	}
	// Convert to seconds since start of day.
	seconds := int64(h*3600 + m*60 + s)
	return &schedulepb.Timestamp{
		Seconds: seconds,
		Nanos:   0,
	}, nil
}

// getSize attempts to determine the size of an io.Reader that also implements
// io.Seeker. If unavailable, it returns 0 and the caller must handle it.
func getSize(r io.Reader) int64 {
	if seeker, ok := r.(io.Seeker); ok {
		cur, _ := seeker.Seek(0, io.SeekCurrent)
		end, _ := seeker.Seek(0, io.SeekEnd)
		_, _ = seeker.Seek(cur, io.SeekStart)
		return end
	}
	return 0
}

// ---------------------------------------------------------------------------
// HTTP handler (optional – useful for micro‑service exposure)
// ---------------------------------------------------------------------------

// TransformHandler is a Gin-compatible HTTP handler that accepts a multipart file
// upload, transforms it, and returns the protobuf binary.
func TransformHandler(c *gin.Context) {
	file, err := c.FormFile("schedule")
	if err != nil {
		logrus.WithError(err).Error("failed to get uploaded file")
		c.JSON(http.StatusBadRequest, gin.H{"error": "schedule file required"})
		return
	}
	f, err := file.Open()
	if err != nil {
		logrus.WithError(err).Error("failed to open uploaded file")
		c.JSON(http.StatusInternalServerError, gin.H{"error": "cannot read file"})
		return
	}
	defer f.Close()

	var buf bytes.Buffer
	if err := TransformFile(c.Request.Context(), file.Filename, &buf); err != nil {
		logrus.WithError(err).Error("transformation failed")
		c.JSON(http.StatusUnprocessableEntity, gin.H{"error": err.Error()})
		return
	}
	c.Data(http.StatusOK, "application/octet-stream", buf.Bytes())
}

// ---------------------------------------------------------------------------
// Example usage (can be removed in production)
// ---------------------------------------------------------------------------

func main() {
	// Simple CLI demo: go run transformer.go <input> <output>
	if len(os.Args) != 3 {
		logrus.Fatal("usage: transformer <source-file> <dest-proto>")
	}
	src := os.Args[1]
	dstPath := os.Args[2]

	outFile, err := os.Create(dstPath)
	if err != nil {
		logrus.WithError(err).Fatal("cannot create destination file")
	}
	defer outFile.Close()

	if err := TransformFile(context.Background(), src, outFile); err != nil {
		logrus.WithError(err).Fatal("transformation failed")
	}
	logrus.Info("transformation completed successfully")
}