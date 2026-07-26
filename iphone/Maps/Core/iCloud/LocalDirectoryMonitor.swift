enum DirectoryMonitorState: CaseIterable, Equatable {
  case started
  case stopped
  case paused
}

protocol DirectoryMonitor: AnyObject {
  var state: DirectoryMonitorState { get }

  func start(completion: ((Result<URL, Error>) -> Void)?)
  func stop()
  func pause()
  func resume()
  /// Observes the directory again and publishes its content, even when nothing has changed.
  func refresh()
}

protocol LocalDirectoryMonitor: DirectoryMonitor {
  var directory: URL { get }
  var delegate: LocalDirectoryMonitorDelegate? { get set }
}

protocol LocalDirectoryMonitorDelegate: AnyObject {
  func didReceiveLocalSnapshot(_ snapshot: LocalSnapshot)
  func didReceiveLocalMonitorError(_ error: Error)
}

final class FileSystemDispatchSourceMonitor: LocalDirectoryMonitor {
  typealias Delegate = LocalDirectoryMonitorDelegate

  fileprivate enum DispatchSourceDebounceState {
    case stopped
    case started(source: DispatchSourceFileSystemObject)
    case debounce(source: DispatchSourceFileSystemObject, timer: Timer)
  }

  private let fileManager: FileManager
  private let fileType: FileType
  private let resourceKeys: [URLResourceKey] = [.nameKey]
  private var dispatchSource: DispatchSourceFileSystemObject?
  private var dispatchSourceDebounceState: DispatchSourceDebounceState = .stopped
  private var dispatchSourceIsSuspended = false
  private var dispatchSourceIsResumed = false

  // MARK: - Public properties

  let directory: URL
  private(set) var state: DirectoryMonitorState = .stopped
  weak var delegate: Delegate?

  init(fileManager: FileManager, directory: URL, fileType: FileType = .kml) throws {
    self.fileManager = fileManager
    self.directory = directory
    self.fileType = fileType
    if !fileManager.fileExists(atPath: directory.path) {
      try fileManager.createDirectory(at: directory, withIntermediateDirectories: true)
    }
  }

  // MARK: - Public methods

  func start(completion: ((Result<URL, Error>) -> Void)? = nil) {
    guard state != .started else { return }

    let nowTimer = Timer.scheduledTimer(withTimeInterval: .zero, repeats: false) { [weak self] _ in
      LOG(.debug, "Initial timer firing...")
      self?.debounceTimerDidFire()
    }

    LOG(.debug, "Start local monitor.")
    if let dispatchSource {
      dispatchSourceDebounceState = .debounce(source: dispatchSource, timer: nowTimer)
      resume()
      completion?(.success(directory))
      return
    }

    do {
      let source = try fileManager.source(for: directory)
      source.setEventHandler { [weak self] in
        self?.queueDidFire()
      }
      dispatchSourceDebounceState = .debounce(source: source, timer: nowTimer)
      source.activate()
      dispatchSource = source
      state = .started
      completion?(.success(directory))
    } catch {
      stop()
      completion?(.failure(error))
    }
  }

  func stop() {
    guard state == .started else { return }
    LOG(.debug, "Stop.")
    suspendDispatchSource()
    dispatchSourceDebounceState = .stopped
    state = .stopped
  }

  func pause() {
    guard state == .started else { return }
    LOG(.debug, "Pause.")
    suspendDispatchSource()
    state = .paused
  }

  func resume() {
    guard state != .started else { return }
    LOG(.debug, "Resume.")
    resumeDispatchSource()
    state = .started
  }

  func refresh() {
    guard state == .started else { return }
    publishSnapshot()
  }

  // MARK: - Private

  private func queueDidFire() {
    let debounceTimeInterval = 0.5
    switch dispatchSourceDebounceState {
    case .started(let source):
      let timer = Timer.scheduledTimer(withTimeInterval: debounceTimeInterval, repeats: false) { [weak self] _ in
        self?.debounceTimerDidFire()
      }
      dispatchSourceDebounceState = .debounce(source: source, timer: timer)
    case .debounce(_, let timer):
      timer.fireDate = Date(timeIntervalSinceNow: debounceTimeInterval)
    // Stay in the `.debounce` state.
    case .stopped:
      // This can happen if the read source fired and enqueued a block on the
      // main queue but, before the main queue got to service that block, someone
      // called `stop()`.  The correct response is to just do nothing.
      break
    }
  }

  private func debounceTimerDidFire() {
    LOG(.debug, "Debounce timer did fire.")
    guard state == .started else {
      LOG(.debug, "State is not started. Skip iteration.")
      return
    }
    guard case .debounce(let source, let timer) = dispatchSourceDebounceState else { fatalError() }
    timer.invalidate()
    dispatchSourceDebounceState = .started(source: source)
    publishSnapshot()
  }

  private func publishSnapshot() {
    do {
      let files = try fileManager
        .contentsOfDirectory(at: directory, includingPropertiesForKeys: [.contentModificationDateKey, .fileSizeKey], options: [.skipsHiddenFiles])
        .filter { $0.pathExtension == fileType.fileExtension }
      // A file whose attributes cannot be read exists but cannot be used: it is not a removed file.
      var unavailableFileNames = Set<String>()
      let items = files.compactMap { fileUrl -> LocalMetadataItem? in
        guard let item = LocalMetadataItem(fileUrl: fileUrl) else {
          unavailableFileNames.insert(fileUrl.lastPathComponent)
          return nil
        }
        return item
      }
      let snapshot = LocalSnapshot(items: items, unavailableFileNames: unavailableFileNames)
      LOG(.debug, "Local contents: \(snapshot.shortDebugDescription)")
      delegate?.didReceiveLocalSnapshot(snapshot)
    } catch {
      delegate?.didReceiveLocalMonitorError(error)
    }
  }

  private func suspendDispatchSource() {
    if !dispatchSourceIsSuspended {
      LOG(.debug, "Suspend dispatch source.")
      dispatchSource?.suspend()
      dispatchSourceIsSuspended = true
      dispatchSourceIsResumed = false
    }
  }

  private func resumeDispatchSource() {
    if !dispatchSourceIsResumed {
      LOG(.debug, "Resume dispatch source.")
      dispatchSource?.resume()
      dispatchSourceIsResumed = true
      dispatchSourceIsSuspended = false
    }
  }
}

private extension FileManager {
  func source(for directory: URL) throws -> DispatchSourceFileSystemObject {
    let directoryFileDescriptor = open(directory.path, O_EVTONLY)
    guard directoryFileDescriptor >= 0 else {
      throw SynchronizationError.failedToOpenLocalDirectoryFileDescriptor
    }
    let dispatchSource = DispatchSource.makeFileSystemObjectSource(fileDescriptor: directoryFileDescriptor, eventMask: [.write], queue: DispatchQueue.main)
    dispatchSource.setCancelHandler {
      close(directoryFileDescriptor)
    }
    return dispatchSource
  }
}
