go
// Package transformer provides utilities to convert various schedule data formats
// (GTFS zip, CSV, JSON) into the protobuf Schedule defined in
// github.com/organicmaps/organicmaps/proto/schedule.
//
// Transformation Rules:
//   1. Detect the source format based on file extension or content sniffing.
//   2. Parse the source into a schedulepb.Schedule protobuf message.
//   3. Apply deterministic ordering to all repeated fields (by ID) to guarantee
//      stable output across runs.
//   4. Marshal the protobuf to binary and write it to the destination writer.
//
// The implementation includes nil checks for all incoming data to avoid panics.
package transformer

import (
	"archive/zip"
	"bytes"
	"context"
	"encoding/csv"
	"errors"
	"io"
	"os"
	"path/filepath"
	"sort"
	"strings"

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
	if r == nil {
		return "", &TransformError{Stage: "detect format", Err: errors.New("nil reader")}
	}
	ext := strings.ToLower(filepath.Ext(filePath))
	switch ext {
	case ".zip":
		return "gtfs", nil
	case ".csv":
		return "csv", nil
	case ".json":
		return "json", nil
	}

	buf := make([]byte, 512)
	n, err := r.Read(buf)
	if err != nil && err != io.EOF {
		return "", &TransformError{Stage: "detect format", Err: err}
	}
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
	if srcPath == "" {
		return &TransformError{Stage: "validate input", Err: errors.New("empty source path")}
	}
	if dst == nil {
		return &TransformError{Stage: "validate input", Err: errors.New("nil destination writer")}
	}
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
		return &TransformError{Stage: "transform", Err: err}
	}
	if schedule == nil {
		return &TransformError{Stage: "transform", Err: errors.New("nil schedule after transformation")}
	}

	// Ensure deterministic ordering of repeated fields.
	deterministicOrder(schedule)

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

// deterministicOrder sorts all repeated fields of Schedule by their ID fields.
func deterministicOrder(s *schedulepb.Schedule) {
	// Agency
	sort.Slice(s.Agency, func(i, j int) bool {
		return s.Agency[i].Id < s.Agency[j].Id
	})
	// Stops
	sort.Slice(s.Stops, func(i, j int) bool {
		return s.Stops[i].Id < s.Stops[j].Id
	})
	// Routes
	sort.Slice(s.Routes, func(i, j int) bool {
		return s.Routes[i].Id < s.Routes[j].Id
	})
	// Trips
	sort.Slice(s.Trips, func(i, j int) bool {
		return s.Trips[i].Id < s.Trips[j].Id
	})
	// StopTimes
	sort.Slice(s.StopTimes, func(i, j int) bool {
		return s.StopTimes[i].Id < s.StopTimes[j].Id
	})
}

// transformGTFS parses a GTFS zip archive and builds a protobuf Schedule.
func transformGTFS(ctx context.Context, r io.Reader) (*schedulepb.Schedule, error) {
	if r == nil {
		return nil, &TransformError{Stage: "gtfs transform", Err: errors.New("nil reader")}
	}
	ra, ok := r.(io.ReaderAt)
	if !ok {
		return nil, &TransformError{Stage: "gtfs unzip", Err: errors.New("reader does not implement io.ReaderAt")}
	}
	size, err := getSize(r)
	if err != nil {
		return nil, &TransformError{Stage: "gtfs size", Err: err}
	}
	zipReader, err := zip.NewReader(ra, size)
	if err != nil {
		return nil, &TransformError{Stage: "gtfs unzip", Err: err}
	}

	schedule := &schedulepb.Schedule{
		Agency:    []*schedulepb.Agency{},
		Stops:     []*schedulepb.Stop{},
		Routes:    []*schedulepb.Route{},
		Trips:     []*schedulepb.Trip{},
		StopTimes: []*schedulepb.StopTime{},
	}

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

	if data, err := readFile("agency.txt"); err == nil {
		if err := parseAgencyCSV(data, schedule); err != nil {
			return nil, &TransformError{Stage: "parse agency.csv", Err: err}
		}
	} else {
		logrus.WithError(err).Debug("agency.txt not present")
	}

	if data, err := readFile("stops.txt"); err == nil {
		if err := parseStopsCSV(data, schedule); err != nil {
			return nil, &TransformError{Stage: "parse stops.csv", Err: err}
		}
	} else {
		return nil, &TransformError{Stage: "read stops.txt", Err: err}
	}

	if data, err := readFile("routes.txt"); err == nil {
		if err := parseRoutesCSV(data, schedule); err != nil {
			return nil, &TransformError{Stage: "parse routes.csv", Err: err}
		}
	} else {
		return nil, &TransformError{Stage: "read routes.txt", Err: err}
	}

	if data, err := readFile("trips.txt"); err == nil {
		if err := parseTripsCSV(data, schedule); err != nil {
			return nil, &TransformError{Stage: "parse trips.csv", Err: err}
		}
	} else {
		return nil, &TransformError{Stage: "read trips.txt", Err: err}
	}

	if data, err := readFile("stop_times.txt"); err == nil {
		if err := parseStopTimesCSV(data, schedule); err != nil {
			return nil, &TransformError{Stage: "parse stop_times.csv", Err: err}
		}
	} else {
		return nil, &TransformError{Stage: "read stop_times.txt", Err: err}
	}

	return schedule, nil
}

// getSize returns the total size of the reader if it implements io.Seeker.
func getSize(r io.Reader) (int64, error) {
	if seeker, ok := r.(io.Seeker); ok {
		cur, err := seeker.Seek(0, io.SeekCurrent)
		if err != nil {
			return 0, err
		}
		end, err := seeker.Seek(0, io.SeekEnd)
		if err != nil {
			return 0, err
		}
		_, _ = seeker.Seek(cur, io.SeekStart)
		return end, nil
	}
	return 0, errors.New("reader does not support seeking")
}

// transformCSV expects a simple CSV with a header line that maps directly to protobuf fields.
func transformCSV(ctx context.Context, r io.Reader) (*schedulepb.Schedule, error) {
	if r == nil {
		return nil, &TransformError{Stage: "csv transform", Err: errors.New("nil reader")}
	}
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
	if r == nil {
		return nil, &TransformError{Stage: "json transform", Err: errors.New("nil reader")}
	}
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

// columnIndexMap creates a map from column name to its index.
func columnIndexMap(header []string) map[string]int {
	m := make(map[string]int, len(header))
	for i, col := range header {
		m[col] = i
	}
	return m
}

// safeGetString returns the string value for a column if it exists, otherwise returns an empty string.
func safeGetString(row []string, idx map[string]int, col string) string {
	if i, ok := idx[col]; ok && i < len(row) {
		return row[i]
	}
	return ""
}

// parseAgencyCSV parses agency.txt into schedule.
func parseAgencyCSV(data []byte, schedule *schedulepb.Schedule) error {
	if len(data) == 0 {
		return errors.New("empty agency data")
	}
	if schedule == nil {
		return errors.New("nil schedule")
	}
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
			Id:       safeGetString(row, idx, "agency_id"),
			Name:     safeGetString(row, idx, "agency_name"),
			Url:      safeGetString(row, idx, "agency_url"),
			Timezone: safeGetString(row, idx, "agency_timezone"),
			Language: safeGetString(row, idx, "agency_lang"),
			Phone:    safeGetString(row, idx, "agency_phone"),
			FareUrl:  safeGetString(row, idx, "agency_fare_url"),
			Email:    safeGetString(row, idx, "agency_email"),
		}
		schedule.Agency = append(schedule.Agency, agency)
	}
	return nil
}

/* ----------------------------------------------------------------------
   Unit Tests for Edge Cases
   ---------------------------------------------------------------------- */