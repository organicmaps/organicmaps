protocol LocalDirectoryMonitorDelegate : AnyObject {
  func didFinishGathering(contents: LocalContents)
  func didUpdate(contents: LocalContents)
}

final class LocalDirectoryMonitor {

  typealias Delegate = LocalDirectoryMonitorDelegate

  fileprivate enum State {
    case stopped
    case started(dirSource: DispatchSourceFileSystemObject)
    case debounce(dirSource: DispatchSourceFileSystemObject, timer: Timer)
  }

  static let `default` = LocalDirectoryMonitor(directory: FileManager.default.bookmarksDirectoryUrl,
                                            matching: kKMLTypeIdentifier,
                                            requestedResourceKeys: [.nameKey])

  private let typeIdentifier: String
  private let requestedResourceKeys: Set<URLResourceKey>
  private let actualResourceKeys: [URLResourceKey]
  private var source: DispatchSourceFileSystemObject?
  private var state: State = .stopped
  private(set) var contents = LocalContents()
  let directory: URL

  weak var delegate: Delegate?

  init(directory: URL, matching typeIdentifier: String, requestedResourceKeys: Set<URLResourceKey>) {
    self.directory = directory
    self.typeIdentifier = typeIdentifier
    self.requestedResourceKeys = requestedResourceKeys
    self.actualResourceKeys = [URLResourceKey](requestedResourceKeys.union([.typeIdentifierKey]))
  }

  // MARK: - Public
  func start() throws {
    guard case .stopped = state else { return }

    if let source {
      source.resume()
      state = .started(dirSource: source)
      return
    }
    
    let directorySource = try LocalDirectoryMonitor.source(for: directory)
    directorySource.setEventHandler { [weak self] in
      self?.queueDidFire()
    }
    directorySource.resume()
    source = directorySource

    let nowTimer = Timer.scheduledTimer(withTimeInterval: 0.0, repeats: false) { [weak self] _ in
      self?.debounceTimerDidFire()
    }

    state = .debounce(dirSource: directorySource, timer: nowTimer)
  }

  func stop() {
    source?.suspend()
    state = .stopped
    contents.removeAll()
  }

  // MARK: - Private
  private static func source(for directory: URL) throws -> DispatchSourceFileSystemObject {
    if !FileManager.default.fileExists(atPath: directory.path) {
      do {
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
      } catch {
        throw error
      }
    }
    let dirFD = open(directory.path, O_EVTONLY)
    // TODO: somtimes it fails on start when app was reinstalled - investigate
    guard dirFD >= 0 else {
      let errorCode = errno
      throw NSError(domain: POSIXError.errorDomain, code: Int(errorCode), userInfo: nil)
    }
    return DispatchSource.makeFileSystemObjectSource(fileDescriptor: dirFD, eventMask: [.write], queue: DispatchQueue.main)
  }

  private func queueDidFire() {
    switch state {
    case .started(let directorySource):
      let timer = Timer.scheduledTimer(withTimeInterval: 0.2, repeats: false) { [weak self] _ in
        self?.debounceTimerDidFire()
      }
      state = .debounce(dirSource: directorySource, timer: timer)
    case .debounce(_, let timer):
      timer.fireDate = Date(timeIntervalSinceNow: 0.2)
      // Stay in the `.debounce` state.
    case .stopped:
      // This can happen if the read source fired and enqueued a block on the
      // main queue but, before the main queue got to service that block, someone
      // called `stop()`.  The correct response is to just do nothing.
      break
    }
  }

  private static func contents(of directory: URL, matching typeIdentifier: String, including: [URLResourceKey]) -> Set<URL> {
    guard let rawContents = try? FileManager.default.contentsOfDirectory(at: directory, includingPropertiesForKeys: including, options: [.skipsHiddenFiles]) else {
      return []
    }
    let filteredContents = rawContents.filter { url in
      guard let type = try? url.resourceValues(forKeys: [.typeIdentifierKey]), let urlType = type.typeIdentifier else {
        return false
      }
      return urlType == typeIdentifier
    }
    return Set(filteredContents)
  }

  private func debounceTimerDidFire() {
    // TODO: add message to fatalError
    guard case .debounce(let dirSource, let timer) = state else { fatalError("") }
    timer.invalidate()
    state = .started(dirSource: dirSource)

    let newContents = LocalDirectoryMonitor.contents(of: directory, matching: typeIdentifier, including: actualResourceKeys)
    var newContentMetadataItems = LocalContents()
    newContents.forEach { url in
      do {
        let metadataItem = try LocalMetadataItem(fileUrl: url)
        newContentMetadataItems.add(metadataItem)
      } catch {
        LOG(.error, "Failed to create LocalMetadataItem: \(error)")
      }
    }

    // When the contentMetadataItems is empty, it means that we are in the initial state.
    if contents.isEmpty {
      delegate?.didFinishGathering(contents: newContentMetadataItems)
    } else {
      delegate?.didUpdate(contents: newContentMetadataItems)
    }
    contents = newContentMetadataItems
  }
}

fileprivate extension LocalDirectoryMonitor.State {
  var isRunning: Bool {
    switch self {
    case .stopped:  return false
    case .started:  return true
    case .debounce: return true
    }
  }
}
