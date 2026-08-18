protocol CloudDirectoryMonitor: DirectoryMonitor {
  var delegate: CloudDirectoryMonitorDelegate? { get set }
  /// Identifies the iCloud account the files belong to.
  var cloudIdentity: Data? { get }

  func fetchUbiquityDirectoryUrl(completion: ((Result<URL, Error>) -> Void)?)
  func isCloudAvailable() -> Bool
}

protocol CloudDirectoryMonitorDelegate: AnyObject {
  func didReceiveCloudSnapshot(_ snapshot: CloudSnapshot)
  func didReceiveCloudMonitorError(_ error: Error)
}

private let kDocumentsDirectoryName = "Documents"

final class iCloudDocumentsMonitor: NSObject, CloudDirectoryMonitor {
  private static let sharedContainerIdentifier: String = {
    var identifier = "iCloud.app.organicmaps"
#if DEBUG
    identifier.append(".debug")
#endif
    return identifier
  }()

  let containerIdentifier: String
  private let fileManager: FileManager
  private let fileType: FileType // TODO: Should be removed when the nested directory support will be implemented
  private var metadataQuery: NSMetadataQuery?
  private var ubiquitousDocumentsDirectory: URL?
  /// Numbers the starts. Asking iCloud where the directory is may take a while, and the monitor can be stopped
  /// or started again meanwhile: a lookup that outlives the start that asked for it must start nothing.
  private var startGeneration = 0

  // MARK: - Public properties

  private(set) var state: DirectoryMonitorState = .stopped
  weak var delegate: CloudDirectoryMonitorDelegate?

  init(fileManager: FileManager = .default, cloudContainerIdentifier: String = iCloudDocumentsMonitor.sharedContainerIdentifier, fileType: FileType) {
    self.fileManager = fileManager
    containerIdentifier = cloudContainerIdentifier
    self.fileType = fileType
    super.init()

    fetchUbiquityDirectoryUrl()
    subscribeOnMetadataQueryNotifications()
  }

  // MARK: - Public methods

  func start(completion: ((Result<URL, Error>) -> Void)? = nil) {
    guard isCloudAvailable() else {
      completion?(.failure(SynchronizationError.iCloudIsNotAvailable))
      return
    }
    // A monitor that is running, or one whose start is in progress, is left alone: it observes the same directory.
    guard state == .stopped else { return }
    state = .starting
    startGeneration += 1
    let generation = startGeneration
    fetchUbiquityDirectoryUrl { [weak self] result in
      // The start this answer belongs to was cancelled by a stop or superseded by a newer one: nothing of it
      // may be started anymore, and whoever asked for it does not wait for the answer.
      guard let self, generation == startGeneration else { return }
      switch result {
      case .failure(let error):
        self.state = .stopped
        completion?(.failure(error))
      case .success(let url):
        LOG(.debug, "Start cloud monitor.")
        self.startQuery()
        self.state = .started
        completion?(.success(url))
      }
    }
  }

  func stop() {
    // The container of another iCloud account is not the one that was looked up, and the account may change
    // while the monitor is stopped: every cached iCloud reference is forgotten here and fetched again on start.
    ubiquitousDocumentsDirectory = nil
    startGeneration += 1
    guard state != .stopped else { return }
    LOG(.debug, "Stop cloud monitor.")
    stopQuery()
    state = .stopped
  }

  func resume() {
    guard state == .paused else { return }
    LOG(.debug, "Resume cloud monitor.")
    metadataQuery?.enableUpdates()
    state = .started
  }

  func pause() {
    guard state == .started else { return }
    LOG(.debug, "Pause cloud monitor.")
    metadataQuery?.disableUpdates()
    state = .paused
  }

  /// Publishes the current query results. Used to check that a file is still missing: iCloud does not notify
  /// about a file that stays absent.
  func refresh() {
    guard state == .started, let metadataQuery, metadataQuery.isStarted else { return }
    publishSnapshot(of: metadataQuery)
  }

  func fetchUbiquityDirectoryUrl(completion: ((Result<URL, Error>) -> Void)? = nil) {
    if let ubiquitousDocumentsDirectory {
      completion?(.success(ubiquitousDocumentsDirectory))
      return
    }
    // The metadata query and the directory URL belong to the main queue: only the lookup itself is done in
    // the background, because it may block until iCloud answers.
    DispatchQueue.global().async {
      guard let containerUrl = self.fileManager.url(forUbiquityContainerIdentifier: self.containerIdentifier) else {
        LOG(.warning, "Failed to retrieve container's URL for:\(self.containerIdentifier)")
        DispatchQueue.main.async { completion?(.failure(SynchronizationError.containerNotFound)) }
        return
      }
      let documentsContainerUrl = containerUrl.appendingPathComponent(kDocumentsDirectoryName, isDirectory: true)
      if !self.fileManager.fileExists(atPath: documentsContainerUrl.path) {
        LOG(.debug, "Creating directory at path: \(documentsContainerUrl.path)...")
        do {
          try self.fileManager.createDirectory(at: documentsContainerUrl, withIntermediateDirectories: true)
        } catch {
          DispatchQueue.main.async { completion?(.failure(SynchronizationError.containerNotFound)) }
          return
        }
      }
      LOG(.debug, "Ubiquity directory URL: \(documentsContainerUrl)")
      DispatchQueue.main.async {
        self.ubiquitousDocumentsDirectory = documentsContainerUrl.standardizedFileURL
        completion?(.success(documentsContainerUrl))
      }
    }
  }

  var cloudIdentity: Data? {
    guard let cloudToken = fileManager.ubiquityIdentityToken else {
      LOG(.warning, "Cloud is not available. Cloud token is nil.")
      return nil
    }
    do {
      return try NSKeyedArchiver.archivedData(withRootObject: cloudToken, requiringSecureCoding: true)
    } catch {
      LOG(.warning, "Failed to archive cloud token: \(error)")
      return nil
    }
  }

  func isCloudAvailable() -> Bool {
    fileManager.ubiquityIdentityToken != nil
  }
}

// MARK: - Private

private extension iCloudDocumentsMonitor {
  // MARK: - MetadataQuery

  func subscribeOnMetadataQueryNotifications() {
    NotificationCenter.default.addObserver(self, selector: #selector(queryDidChange(_:)), name: NSNotification.Name.NSMetadataQueryDidFinishGathering, object: nil)
    NotificationCenter.default.addObserver(self, selector: #selector(queryDidChange(_:)), name: NSNotification.Name.NSMetadataQueryDidUpdate, object: nil)
  }

  func startQuery() {
    metadataQuery = Self.buildMetadataQuery(for: fileType)
    guard let metadataQuery, !metadataQuery.isStarted else { return }
    LOG(.debug, "Start metadata query")
    metadataQuery.start()
  }

  func stopQuery() {
    LOG(.debug, "Stop metadata query")
    metadataQuery?.stop()
    metadataQuery = nil
  }

  /** The notification's `added`, `changed` and `removed` lists describe how the query results changed and not
   what the user did: iCloud reports a file as removed while it is being replaced, reindexed or trashed.
   Only the complete list of the current results is published, and the synchronization decides what changed. */
  @objc func queryDidChange(_ notification: Notification) {
    guard isCloudAvailable(), let metadataQuery, (notification.object as? NSMetadataQuery) === metadataQuery else { return }
    publishSnapshot(of: metadataQuery)
  }

  func publishSnapshot(of metadataQuery: NSMetadataQuery) {
    guard let ubiquitousDocumentsDirectory else {
      delegate?.didReceiveCloudMonitorError(SynchronizationError.containerNotFound)
      return
    }
    metadataQuery.disableUpdates()
    defer { metadataQuery.enableUpdates() }

    let snapshot = Self.snapshot(of: metadataQuery, in: ubiquitousDocumentsDirectory)
    LOG(.debug, "Cloud contents: \(snapshot.shortDebugDescription)")
    delegate?.didReceiveCloudSnapshot(snapshot)
  }

  static func buildMetadataQuery(for fileType: FileType) -> NSMetadataQuery {
    let metadataQuery = NSMetadataQuery()
    metadataQuery.notificationBatchingInterval = 1
    metadataQuery.searchScopes = [NSMetadataQueryUbiquitousDocumentsScope]
    metadataQuery.predicate = NSPredicate(format: "%K LIKE %@", NSMetadataItemFSNameKey, "*.\(fileType.fileExtension)")
    metadataQuery.sortDescriptors = [NSSortDescriptor(key: NSMetadataItemFSNameKey, ascending: true)]
    return metadataQuery
  }
}

extension iCloudDocumentsMonitor {
  static func snapshot(of metadataQuery: NSMetadataQuery, in directory: URL) -> CloudSnapshot {
    var items = CloudContents()
    var unavailableFileNames = Set<String>()
    var isComplete = true
    for case let metadataItem as NSMetadataItem in metadataQuery.results {
      switch CloudMetadataItem.observation(from: metadataItem) {
      case .actionable(let item):
        // Files in the trash and in nested directories are not synchronized and are not a part of the directory.
        guard item.fileUrl.isInside(directory) else { continue }
        items.append(item)
      case .unusable(let fileName, let fileUrl, let missingAttributes):
        /* The same holds for a file that is only known to exist: reported under its bare name, a copy iCloud is
         moving to the trash would stand for the file in the directory and keep it from ever being confirmed as
         gone. An item without a URL cannot be placed anywhere, so it counts for the directory it was found in. */
        guard fileUrl.map({ $0.isInside(directory) }) ?? true else { continue }
        LOG(.warning, "iCloud file \(fileName) is not available: no \(missingAttributes.joined(separator: ", "))")
        unavailableFileNames.insert(fileName)
      case .unidentifiable(let missingAttributes):
        LOG(.warning, "Unidentifiable iCloud item: no \(missingAttributes.joined(separator: ", "))")
        isComplete = false
      }
    }
    return CloudSnapshot(items: items, unavailableFileNames: unavailableFileNames, isComplete: isComplete)
  }
}

private extension URL {
  /// Paths are compared, not URLs: whether appendingPathComponent produced a trailing slash depends on the
  /// directory already existing when the URL was built, and on the first launch it does not.
  func isInside(_ directory: URL) -> Bool {
    deletingLastPathComponent().path == directory.path
  }
}
