// backend/storage/store.go
//
// Package storage implements a thin wrapper around RocksDB for persisting
// transformed public‑transport schedules. It provides simple, thread‑safe
// read/write methods, graceful error handling and structured logging.
//
// The implementation is deliberately minimal – it stores opaque byte slices
// (e.g. protobuf or flatbuffer messages) identified by a string key.
// All callers are responsible for encoding/decoding the payload.
//
// Dependencies:
//   - github.com/tecbot/gorocksdb v0.1.0+
//   - github.com/sirupsen/logrus   (optional, can be replaced with std log)
//   - Go 1.22 or newer
//
package storage

import (
	"context"
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"sync"

	"github.com/sirupsen/logrus"
	"github.com/tecbot/gorocksdb"
)

// Store wraps a RocksDB instance and provides safe concurrent access.
type Store struct {
	db     *gorocksdb.DB
	ro     *gorocksdb.ReadOptions
	wo     *gorocksdb.WriteOptions
	logger *logrus.Logger
	mu     sync.RWMutex // protects db during close/reopen
}

// New creates ( new Store instance. The database directory will be created
// if it does not exist. The returned Store must be closed with Close().
func New(ctx context.Context, dbPath string, logger *logrus.Logger) (*Store, error) {
	if logger == nil {
		logger = logrus.New()
	}
	// Ensure the directory exists.
	if err := os.MkdirAll(dbPath, 0o755); err != nil {
		return nil, fmt.Errorf("failed to create db directory %s: %w", dbPath, err)
	}

	// RocksDB options.
	opts := gorocksdb.NewDefaultOptions()
	opts.SetCreateIfMissing(true)
	opts.SetErrorIfExists(false)
	// Tune for read‑heavy workload; these values can be overridden later.
	opts.SetWriteBufferSize(64 * 1024 * 1024) // 64 MiB
	opts.SetMaxOpenFiles(1000)
	opts.SetCompression(gorocksdb.SnappyCompression)

	db, err := gorocksdb.OpenDb(opts, dbPath)
	if err != nil {
		return nil, fmt.Errorf("failed to open rocksdb at %s: %w", dbPath, err)
	}
	logger.Infof("RocksDB opened at %s", dbPath)

	return &Store{
		db:     db,
		ro:     gorocksdb.NewDefaultReadOptions(),
		wo:     gorocksdb.NewDefaultWriteOptions(),
		logger: logger,
	}, nil
}

// PutSchedule stores a schedule payload under the given key.
// The key must be a non‑empty string. The payload is stored as‑is.
func (s *Store) PutSchedule(ctx context.Context, key string, payload []byte) error {
	if key == "" {
		return errors.New("key cannot be empty")
	}
	if payload == nil {
		return errors.New("payload cannot be nil")
	}

	s.mu.RLock()
	defer s.mu.RUnlock()

	// Ensure DB is still open.
	if s.db == nil {
		return errors.New("store is closed")
	}

	// Write atomically.
	err := s.db.Put(s.wo, []byte(key), payload)
	if err != nil {
		s.logger.WithError(err).WithField("key", key).Error("failed to write schedule")
		return fmt.Errorf("rocksdb put failed for key %s: %w", key, err)
	}
	s.logger.WithField("key", key).Debug("schedule written")
	return nil
}

// GetSchedule retrieves a schedule payload for the given key.
// Returns ErrNotFound if the key does not exist.
func (s *Store) GetSchedule(ctx context.Context, key string) ([]byte, error) {
	if key == "" {
		return nil, errors.New("key cannot be empty")
	}

	s.mu.RLock()
	defer s.mu.RUnlock()

	if s.db == nil {
		return nil, errors.New("store is closed")
	}

	slice, err := s.db.Get(s.ro, []byte(key))
	if err != nil {
		s.logger.WithError(err).WithField("key", key).Error("failed to read schedule")
		return nil, fmt.Errorf("rocksdb get failed for key %s: %w", key, err)
	}
	defer slice.Free()

	if !slice.Exists() {
		return nil, fmt.Errorf("key %s not found: %w", key, ErrNotFound)
	}
	s.logger.WithField("key", key).Debug("schedule read")
	return slice.Data(), nil
}

// DeleteSchedule removes a schedule entry by key.
func (s *Store) DeleteSchedule(ctx context.Context, key string) error {
	if key == "" {
		return errors.New("key cannot be empty")
	}

	s.mu.RLock()
	defer s.mu.RUnlock()

	if s.db == nil {
		return errors.New("store is closed")
	}

	err := s.db.Delete(s.wo, []byte(key))
	if err != nil {
		s.logger.WithError(err).WithField("key", key).Error("failed to delete schedule")
		return fmt.Errorf("rocksdb delete failed for key %s: %w", key, err)
	}
	s.logger.WithField("key", key).Debug("schedule deleted")
	return nil
}

// Close gracefully shuts down the RocksDB instance.
// It is safe to call multiple times; subsequent calls become no‑ops.
func (s *Store) Close() error {
	s.mu.Lock()
	defer s.mu.Unlock()

	if s.db == nil {
		return nil // already closed
	}
	s.db.Close()
	s.ro.Destroy()
	s.wo.Destroy()
	s.db = nil
	s.logger.Info("RocksDB closed")
	return nil
}

// ErrNotFound signals that a requested key does not exist in the store.
var ErrNotFound = errors.New("record not found")

// ---------------------------------------------------------------------
// Helper utilities – these are optional but useful in a production
// environment and are kept in the same file for simplicity.
// ---------------------------------------------------------------------

// ListKeys returns all keys currently stored in the database.
// It is primarily intended for debugging or administrative tooling.
func (s *Store) ListKeys(ctx context.Context) ([]string, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	if s.db == nil {
		return nil, errors.New("store is closed")
	}

	iter := s.db.NewIterator(s.ro)
	defer iter.Close()

	var keys []string
	for iter.SeekToFirst(); iter.Valid(); iter.Next() {
		keys = append(keys, string(iter.Key().Data()))
	}
	if err := iter.Err(); err != nil {
		s.logger.WithError(err).Error("iteration error while listing keys")
		return nil, fmt.Errorf("rocksdb iteration error: %w", err)
	}
	return keys, nil
}

// DumpDB creates a snapshot of the entire database in a human‑readable
// format (key -> hex payload). This function is intended for diagnostics
// and should not be used in the hot path.
func (s *Store) DumpDB(ctx context.Context, outPath string) error {
	s.mu.RLock()
	defer s.mu.RUnlock()

	if s.db == nil {
		return errors.New("store is closed")
	}

	f, err := os.Create(outPath)
	if err != nil {
		return fmt.Errorf("cannot create dump file %s: %w", outPath, err)
	}
	defer f.Close()

	iter := s.db.NewIterator(s.ro)
	defer iter.Close()

	for iter.SeekToFirst(); iter.Valid(); iter.Next() {
		_, wErr := fmt.Fprintf(f, "%s: %x\n", iter.Key().Data(), iter.Value().Data())
		if wErr != nil {
			return fmt.Errorf("write error while dumping db: %w", wErr)
		}
	}
	if err := iter.Err(); err != nil {
		return fmt.Errorf("rocksdb iteration error during dump: %w", err)
	}
	s.logger.WithField("path", outPath).Info("database dump completed")
	return nil
}

// ---------------------------------------------------------------------
// Context‑aware helper – cancels ongoing I/O if the context is done.
// ---------------------------------------------------------------------

// withCancel checks the context before performing a DB operation.
// It returns an error if the context has been cancelled.
func withCancel(ctx context.Context) error {
	select {
	case <-ctx.Done():
		return ctx.Err()
	default:
		return nil
	}
}