package main

import (
	"fmt"
	"io"
	"math"
	"net/http"
	"sync"
	"time"
)

type SourceStatus int

const (
	SOURCE_IDLE SourceStatus = iota
	SOURCE_BUSY
	SOURCE_BAD
	SOURCE_FAILED
)

type FetchSource struct {
	Manager *FetchManager
	Id      string
	Host    string
	// Boost determines how much we boost this source's measured speed
	// at selection time.
	Boost float64
	// SpeedLimit is the maximal speed in kilobytes per second that
	// we should be fetching from this source. 0 means no limit.
	SpeedLimit       float64
	Status           SourceStatus
	ChunkSpeed       float64
	LastRecordedSpeed float64
	LastSetTime      time.Time
	TotalDownloaded  int64
	NumChunkAttempts int
	NumRetries       int
	IsCDN            bool
	SlowThreshold    float64
}

func NewFetchSource(
	manager *FetchManager, id string, host string,
	chunkSpeed float64, speedLimit float64, boost float64, isCDN bool, slowThreshold float64) *FetchSource {
	return &FetchSource{
		Manager:        manager,
		Id:             id,
		Host:           host,
		Boost:          boost,
		SpeedLimit:     speedLimit,
		Status:         SOURCE_IDLE,
		ChunkSpeed:     chunkSpeed,
		LastSetTime:    time.Now(),
		NumRetries:     0,
		IsCDN:          isCDN,
		SlowThreshold:  slowThreshold,
	}
}

func (s *FetchSource) RecordChunkSpeed(seconds, speed float64) {
	// We must have the decay otherwise each slow chunk throws off a
	// well-performing server for a very long time.
	decay := float64(0.5)
	s.ChunkSpeed = (1 - decay) * speed + decay * s.ChunkSpeed
	s.LastSetTime = time.Now()
	s.LastRecordedSpeed = speed

	s.Manager.NumTotalChunks += 1
	s.Manager.TimeTotalChunks += seconds

	maxRecordedChunks := 10.0
	if (s.Manager.NumTotalChunks > maxRecordedChunks) { // TODO: magic number, maximum number of recorded chunks (for decay)
		s.Manager.TimeTotalChunks = s.Manager.TimeTotalChunks * maxRecordedChunks / s.Manager.NumTotalChunks
		s.Manager.NumTotalChunks = maxRecordedChunks
	}
}

func (s *FetchSource) RecordError(err error) {
	s.ChunkSpeed = s.Manager.AverageChunkSpeed()
	s.LastSetTime = time.Now()
	s.NumRetries += 1
	s.Status = SOURCE_BAD
	if s.NumRetries >= 3 {  // TODO: Magic number
		s.Status = SOURCE_FAILED
	}
}

func (s *FetchSource) Score() float64 {
	timeSinceLastRecording := time.Now().Sub(s.LastSetTime)

	errorPenalty := float64(1.0)
	if s.Status == SOURCE_BAD {
		penaltyTime := time.Second * time.Duration(20 * s.Manager.AverageChunkTime()) // TODO: 20 - magic number, number of chunks to wait before checking server again
		errorPenalty = 1.0 * s.Manager.UncertaintyBoost(timeSinceLastRecording - penaltyTime)
	}
	return s.EstimatedSpeed() * s.Boost * errorPenalty
}

func (s *FetchSource) EstimatedSpeed() float64 {
	timeSinceLastRecording := time.Now().Sub(s.LastSetTime)
	uncertaintyBoost := s.Manager.UncertaintyBoost(timeSinceLastRecording)
	estimatedSpeed := s.ChunkSpeed * uncertaintyBoost
	if s.SpeedLimit != 0 && estimatedSpeed > s.SpeedLimit {
		estimatedSpeed = s.SpeedLimit
	}
	return estimatedSpeed
}

type FetchManager struct {
	SourceMutex                  sync.Mutex
	Sources                      []*FetchSource
	ChunkSize                    int64
	NumWorkers                   int
	UncertaintyBoostPerChunkTime float64

	NumTotalChunks  float64
	TimeTotalChunks float64
}

func (m *FetchManager) CreateScheduler(path string, size int64) *FetchScheduler {
	return &FetchScheduler{
		Client:   http.Client{},
		Manager:  m,
		Path:     path,
		FileSize: size,
	}
}

func (m *FetchManager) IsCDNEnabled() bool {
	for _, source := range m.Sources {
		if source.IsCDN {
			continue
		}
		if source.Status == SOURCE_FAILED {
			continue
		}
		if source.ChunkSpeed < source.SlowThreshold {
			continue
		}
		return false
	}
	return true
}

func (m *FetchManager) PrintSources() {
	for _, source := range m.Sources {
		fmt.Printf("%v, status=%d spd=%5.0f espd=%5.0f last_speed=%5.0f score=%5.2f uncertaintyBoost=%5.2f, Total=%5.0f Attempts=%d, timeSinceLastSet=%5.1f, numRetries=%d, isCDN=%v\n",
			source.Id,
			source.Status,
			source.ChunkSpeed/1024,
			source.EstimatedSpeed()/1024,
			source.LastRecordedSpeed/1024,
			source.Score()/1024/1024,
			m.UncertaintyBoost(time.Now().Sub(source.LastSetTime)),
			float64(source.TotalDownloaded)/1024.0,
			source.NumChunkAttempts,
			time.Now().Sub(source.LastSetTime).Seconds(),
			source.NumRetries,
			source.IsCDN,
		)
	}
	if m.NumTotalChunks != 0 {
		fmt.Printf("Average chunk time=%.2f, avg chunk speed=%.1f KBps\n", m.AverageChunkTime(), m.AverageChunkSpeed() / 1024)
	}
	fmt.Printf("CDN is Enabled: %v\n", m.IsCDNEnabled())
	fmt.Println()
}

func (m *FetchManager) UncertaintyBoost(duration time.Duration) float64 {
	chunks := duration.Seconds() / m.AverageChunkTime()
	return math.Pow(m.UncertaintyBoostPerChunkTime, chunks)
}

func (m *FetchManager) AverageChunkTime() float64 {
	if m.NumTotalChunks == 0 {
		return 1
	}
	return m.TimeTotalChunks / m.NumTotalChunks
}

func (m *FetchManager) AverageChunkSpeed() float64 {
	return float64(m.ChunkSize) / m.AverageChunkTime()
}

// GetSource finds an optimal source to use for fetching.
func (m *FetchManager) GetSource() *FetchSource {
	m.SourceMutex.Lock()
	defer m.SourceMutex.Unlock()

	m.PrintSources()
	
	isCDNEnabled := m.IsCDNEnabled()

	var selectedSource *FetchSource
	for _, source := range m.Sources {
		if source.IsCDN && !isCDNEnabled {
			continue
		}
		switch source.Status {
			case SOURCE_BUSY, SOURCE_FAILED:
				continue
			case SOURCE_BAD:
				if time.Now().Sub(source.LastSetTime).Seconds() < 20 * m.AverageChunkTime() {
					continue
				}
		}
		if selectedSource == nil {
			selectedSource = source
			continue
		}
		if source.Score() > selectedSource.Score() {
			selectedSource = source
		}
	}

	if selectedSource != nil {
		//fmt.Printf("Selected source %+v\n", *selectedSource)
		selectedSource.Status = SOURCE_BUSY
		selectedSource.NumChunkAttempts++
	} else {
		//fmt.Printf("Source not found\n")
		// TODO
	}
	return selectedSource
}

func (m *FetchManager) ReleaseSource(source *FetchSource) {
	m.SourceMutex.Lock()
	defer m.SourceMutex.Unlock()
	
	source.Status = SOURCE_IDLE
	source.NumRetries = 0
}

type FetchScheduler struct {
	Manager             *FetchManager
	Client              http.Client
	FileSize            int64
	Path                string
	taskMutex           sync.Mutex
	tasks               []*FetchSchedulerTask
	numOutstandingTasks int
	TaskDoneCh          chan *FetchSchedulerTask
	WorkerDoneCh        chan bool
}

func minInt64(a, b int64) int64 {
	if a < b {
		return a
	}
	return b
}

func (f *FetchScheduler) Fetch() {
	// Create all the tasks.
	for pos := int64(0); pos < f.FileSize; pos += f.Manager.ChunkSize {
		f.tasks = append(f.tasks, &FetchSchedulerTask{
			scheduler: f,
			startPos:  pos,
			endPos:    minInt64(pos + f.Manager.ChunkSize, f.FileSize),
		})
		f.numOutstandingTasks++
	}
	// Start the workers in the worker pool.
	f.TaskDoneCh = make(chan *FetchSchedulerTask)
	f.WorkerDoneCh = make(chan bool)
	for i := 0; i < f.Manager.NumWorkers; i++ {
		go f.WorkerLoop()
	}
	// Wait for all the tasks to complete.
	numTasksDone := 0
	numWorkersDone := 0
	for {
		select {
			case <-f.TaskDoneCh:
				numTasksDone += 1
				if numTasksDone == f.numOutstandingTasks {
					fmt.Println("Loading successful")
					return
				}
			case <-f.WorkerDoneCh:
				numWorkersDone += 1
				if numWorkersDone == f.Manager.NumWorkers {
					fmt.Println("Loading failed")
					return
				}
		}
	}
}

func (f *FetchScheduler) GetTask() *FetchSchedulerTask {
	f.taskMutex.Lock()
	defer f.taskMutex.Unlock()

	if len(f.tasks) == 0 {
		return nil
	}
	task := f.tasks[0]
	f.tasks = f.tasks[1:]
	return task
}

func (m *FetchManager) AllSourcesFailed() bool {
	for _, source := range m.Sources {
		if source.Status != SOURCE_FAILED {
			return false
		}
	}
	return true
}

func (f *FetchScheduler) WorkerLoop() {
	for {
		if f.Manager.AllSourcesFailed() {
			f.WorkerDoneCh <- true
		}
		task := f.GetTask()
		if task == nil {
			return
		}
		task.Run()
		f.TaskDoneCh <- task
	}
}

type FetchSchedulerTask struct {
	scheduler *FetchScheduler
	startPos  int64
	endPos    int64
	startTime time.Time
	endTime   time.Time
	req       http.Request
	resp      http.Response
}

func (t *FetchSchedulerTask) Run() {
	// TODO: If there is an error, Run() must be re-attempted several times for each task,
	// with different sources.
	var source *FetchSource
	for {
		source = t.scheduler.Manager.GetSource()
		if source == nil {
			if t.scheduler.Manager.AllSourcesFailed() {
				return
			}
			fmt.Printf("Cannot acquire new source")
			time.Sleep(time.Second * time.Duration(t.scheduler.Manager.AverageChunkTime()) * 3) // TODO: magic numbers
		} else {
			break
		}
	}

	t.startTime = time.Now()
	err := t.RunWithSource(source)
	if err != nil {
		fmt.Printf("Error: %v\n", err)
		source.RecordError(err)
		return
	}

	t.endTime = time.Now()

	loadDuration := t.endTime.Sub(t.startTime)
	speed := float64(t.endPos-t.startPos) / loadDuration.Seconds()
	source.RecordChunkSpeed(loadDuration.Seconds(), speed)
	source.TotalDownloaded += t.endPos - t.startPos
	t.scheduler.Manager.ReleaseSource(source)

	fmt.Printf("Done fetching task %d-%d, from %v, speed %.2f\n",
		t.startPos, t.endPos, source.Host, speed/1024)
}

func (t *FetchSchedulerTask) RunWithSource(source *FetchSource) error {
	url := "http://" + source.Host + t.scheduler.Path
	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return err
	}
	req.Header.Add("Range", fmt.Sprintf("bytes=%d-%d", t.startPos, t.endPos))
	resp, err := t.scheduler.Client.Do(req)
	if err != nil {
		return err
	}
	if resp.StatusCode / 100 != 2 {
		return fmt.Errorf("HTTP status %v for %v", resp.Status, url)
	}

	// Speed limit algorithm works by limiting the maximal rate at which we
	// agree to read from this connection. With that, we trust the normal
	// buffering of the OS to perform reasonably efficiently wrt. packet transfer.
	var speedLimitTicker *time.Ticker
	bufferSize := 16*1024
	if source.SpeedLimit != 0 {  // TODO: check if needed in production
		tickInterval := time.Duration(float64(time.Second) * float64(bufferSize) / source.SpeedLimit)
		speedLimitTicker = time.NewTicker(tickInterval)
		defer speedLimitTicker.Stop()
		//fmt.Printf("Limit for %v=%.0f bps, Interval = %.3f sec\n",
		//	source.Id, source.SpeedLimit, tickInterval.Seconds())
	}

	reader := resp.Body
	buf := make([]byte, bufferSize)
	var bytesRead int64
	for {
		if speedLimitTicker != nil {
			//start := time.Now()
			<-speedLimitTicker.C
			//fmt.Printf("%v Waited for ticker: %.2f sec\n",
			//	source.Id, time.Now().Sub(start).Seconds())
		}
		n, err := reader.Read(buf)
		if err != nil && err != io.EOF {
			return err
		}
		if n != 0 {
			// One buffer of a chunk has been loaded.
			//fmt.Printf("%v Got %d bytes\n", source.Id, n)
			bytesRead += int64(n)
		} else {
			break
		}
// 		elapsedTime := time.Now().Sub(t.startTime).Seconds()
// 		momentarySpeed := float64(bytesRead) / 1024 / (elapsedTime + 1e-100)
		//fmt.Printf("%v Momentary speed at %.2f: %.2f\n", source.Id, elapsedTime, momentarySpeed )

// 		minAllowedSpeed := t.scheduler.Manager.AverageChunkSpeed() / 20 // TODO: magic numbers, review, maybe mark server as bad
// 		if elapsedTime > 16 && momentarySpeed < minAllowedSpeed {
// 			fmt.Printf("Server %v too slow", source.Id)
// 		}
	}

	return nil
}

func main() {
	manager := &FetchManager{
		ChunkSize:                    256 * 1024,
		NumWorkers:                   3,
		UncertaintyBoostPerChunkTime: 1.1,
	}
	manager.Sources = []*FetchSource{
		NewFetchSource(manager, "2", "second.server", 800*1024, 1000*1024, 3.0, false, 10000 * 1024),
	/*	NewFetchSource(manager, "3.1", "third.server", 350*1024, 0*1024, 1.0),
		NewFetchSource(manager, "3.2", "third.server", 350*1024, 0*1024, 1.0),
		NewFetchSource(manager, "3.3", "third.server", 350*1024, 0*1024, 1.0),
		NewFetchSource(manager, "3.4", "third.server", 350*1024, 0*1024, 1.0),
		NewFetchSource(manager, "3.5", "third.server", 350*1024, 0*1024, 1.0),
		NewFetchSource(manager, "3.6", "third.server", 350*1024, 0*1024, 1.0),
		NewFetchSource(manager, "3.7", "third.server", 350*1024, 0*1024, 1.0),*/
		NewFetchSource(manager, "3", "third.server", 350*1024, 0*1024, 1.0, false, 10000 * 1024),
		NewFetchSource(manager, "4", "fourth.server", 160*1024, 0*1024, 1.0, false, 10000 * 1024),
		NewFetchSource(manager, "1", "first.server", 50*1024, 0*1024, 1.0, false, 10000 * 1024),
		NewFetchSource(manager, "5", "fifth.server", 1500*1024, 0*1024, 1.0, false, 10000 * 1024),
		NewFetchSource(manager, "us3-cdn", "mappy.local", 150*1024, 0*1024, 0.1, true, 10 * 1024),
		
	}

	for i := 0; i < 10; i++ {
		scheduler := manager.CreateScheduler("/ios/131031/Belarus.mwm", 67215188)
		scheduler.Fetch()
	}
}
