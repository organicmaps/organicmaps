protocol CloudDirectoryMonitor: DirectoryMonitor {
  var fileManager: FileManager { get }
  var ubiquitousDocumentsDirectory: URL? { get }
  var delegate: CloudDirectoryMonitorDelegate? { get set }

  func fetchUbiquityDirectoryUrl(completion: ((Result<URL, Error>) -> Void)?)
  func isCloudAvailable() -> Bool
}

protocol CloudDirectoryMonitorDelegate : AnyObject {
  func didFinishGathering(contents: CloudContents)
  func didUpdate(contents: CloudContents)
  func didReceiveCloudMonitorError(_ error: Error)
}

private let kUDCloudIdentityKey = "com.apple.organicmaps.UbiquityIdentityToken"
private let kDocumentsDirectoryName = "Documents"

class iCloudDocumentsDirectoryMonitor: NSObject, CloudDirectoryMonitor {

  static let sharedContainerIdentifier: String = {
    var identifier = "iCloud.app.organicmaps"
    #if DEBUG
    identifier.append(".debug")
    #endif
    return identifier
  }()

  let containerIdentifier: String
  let fileManager: FileManager
  private let fileType: FileType // TODO: Should be removed when the nested directory support will be implemented
  private(set) var metadataQuery: NSMetadataQuery?
  private(set) var ubiquitousDocumentsDirectory: URL?

  // MARK: - Public properties
  private(set) var state: DirectoryMonitorState = .stopped
  weak var delegate: CloudDirectoryMonitorDelegate?

  init(fileManager: FileManager = .default, cloudContainerIdentifier: String = iCloudDocumentsDirectoryMonitor.sharedContainerIdentifier, fileType: FileType) {
    self.fileManager = fileManager
    self.containerIdentifier = cloudContainerIdentifier
    self.fileType = fileType
    super.init()

    fetchUbiquityDirectoryUrl()
    subscribeOnMetadataQueryNotifications()
    subscribeOnCloudAvailabilityNotifications()
  }

  // MARK: - Public methods
  func start(completion: ((Result<URL, Error>) -> Void)? = nil) {
    guard isCloudAvailable() else {
      completion?(.failure(SynchronizationError.iCloudIsNotAvailable))
      return
    }
    fetchUbiquityDirectoryUrl { [weak self] result in
      guard let self else { return }
      switch result {
      case .failure(let error):
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
    guard state != .stopped else { return }
    LOG(.debug, "Stop cloud monitor.")
    stopQuery()
    state = .stopped
  }

  func resume() {
    guard state != .started else { return }
    LOG(.debug, "Resume cloud monitor.")
    metadataQuery?.enableUpdates()
    state = .started
  }

  func pause() {
    guard state != .paused else { return }
    LOG(.debug, "Pause cloud monitor.")
    metadataQuery?.disableUpdates()
    state = .paused
  }

  func fetchUbiquityDirectoryUrl(completion: ((Result<URL, Error>) -> Void)? = nil) {
    if let ubiquitousDocumentsDirectory {
      completion?(.success(ubiquitousDocumentsDirectory))
      return
    }
    DispatchQueue.global().async {
      guard let containerUrl = self.fileManager.url(forUbiquityContainerIdentifier: self.containerIdentifier) else {
        LOG(.warning, "Failed to retrieve container's URL for:\(self.containerIdentifier)")
        completion?(.failure(SynchronizationError.containerNotFound))
        return
      }
      let documentsContainerUrl = containerUrl.appendingPathComponent(kDocumentsDirectoryName)
      if !self.fileManager.fileExists(atPath: documentsContainerUrl.path) {
        LOG(.debug, "Creating directory at path: \(documentsContainerUrl.path)...")
        do {
          try self.fileManager.createDirectory(at: documentsContainerUrl, withIntermediateDirectories: true)
        } catch {
          completion?(.failure(SynchronizationError.containerNotFound))
        }
      }
      LOG(.debug, "Ubiquity directory URL: \(documentsContainerUrl)")
      self.ubiquitousDocumentsDirectory = documentsContainerUrl
      completion?(.success(documentsContainerUrl))
    }
  }

  func isCloudAvailable() -> Bool {
    let cloudToken = fileManager.ubiquityIdentityToken
    guard let cloudToken else {
      UserDefaults.standard.removeObject(forKey: kUDCloudIdentityKey)
      LOG(.warning, "Cloud is not available. Cloud token is nil.")
      return false
    }
    do {
      let data = try NSKeyedArchiver.archivedData(withRootObject: cloudToken, requiringSecureCoding: true)
      UserDefaults.standard.set(data, forKey: kUDCloudIdentityKey)
      return true
    } catch {
      UserDefaults.standard.removeObject(forKey: kUDCloudIdentityKey)
      LOG(.warning, "Failed to archive cloud token: \(error)")
      return false
    }
  }

  class func buildMetadataQuery(for fileType: FileType) -> NSMetadataQuery {
    let metadataQuery = NSMetadataQuery()
    metadataQuery.notificationBatchingInterval = 1
    metadataQuery.searchScopes = [NSMetadataQueryUbiquitousDocumentsScope]
    metadataQuery.predicate = NSPredicate(format: "%K LIKE %@", NSMetadataItemFSNameKey, "*.\(fileType.fileExtension)")
    metadataQuery.sortDescriptors = [NSSortDescriptor(key: NSMetadataItemFSNameKey, ascending: true)]
    return metadataQuery
  }

  class func getContentsFromNotification(_ notification: Notification) throws -> CloudContents {
    guard let metadataQuery = notification.object as? NSMetadataQuery,
          let metadataItems = metadataQuery.results as? [NSMetadataItem] else {
      throw SynchronizationError.failedToRetrieveMetadataQueryContent
    }
    return try metadataItems.map { try CloudMetadataItem(metadataItem: $0) }
  }

  // There are no ways to retrieve the content of iCloud's .Trash directory on the macOS because it uses different file system and place trashed content in the /Users/<user_name>/.Trash which cannot be observed without access.
  // When we get a new notification and retrieve the metadata from the object the actual list of items in iOS contains both current and deleted files (which is in .Trash/ directory now) but on macOS we only have absence of the file. So there are no way to get list of deleted items on macOS on didFinishGathering state.
  // Due to didUpdate state we can get the list of deleted items on macOS from the userInfo property but cannot get their new url.
  class func getTrashContentsFromNotification(_ notification: Notification) throws -> CloudContents {
    guard let removedItems = notification.userInfo?[NSMetadataQueryUpdateRemovedItemsKey] as? [NSMetadataItem] else {
      LOG(.warning, "userInfo[NSMetadataQueryUpdateRemovedItemsKey] is nil")
      return []
    }
    LOG(.info, "Removed from the cloud content: \n\(removedItems.shortDebugDescription)")
    return try removedItems.map { try CloudMetadataItem(metadataItem: $0, isRemoved: true) }
  }

  class func getTrashedContentsFromTrashDirectory(fileManager: FileManager, ubiquitousDocumentsDirectory: URL) throws -> CloudContents {
    // There are no ways to retrieve the content of iCloud's .Trash directory on macOS.
    if #available(iOS 14.0, *), ProcessInfo.processInfo.isiOSAppOnMac {
      LOG(.warning, "Trashed content is not available on macOS.")
      return []
    }
    let trashDirectoryUrl = try fileManager.trashDirectoryUrl(for: ubiquitousDocumentsDirectory)
    let removedItems = try fileManager.contentsOfDirectory(at: trashDirectoryUrl,
                                                           includingPropertiesForKeys: [.isDirectoryKey],
                                                           options: [.skipsPackageDescendants, .skipsSubdirectoryDescendants])
    LOG(.info, "Trashed cloud content: \n\(removedItems)")
    return try removedItems.map { try CloudMetadataItem(fileUrl: $0, isRemoved: true) }
  }
}

// MARK: - Private
private extension iCloudDocumentsDirectoryMonitor {

  func subscribeOnCloudAvailabilityNotifications() {
    NotificationCenter.default.addObserver(self, selector: #selector(cloudAvailabilityChanged(_:)), name: .NSUbiquityIdentityDidChange, object: nil)
  }

  // TODO: - Actually this notification was never called. If user disable the iCloud for the current app during the active state the app will be relaunched. Needs to investigate additional cases when this notification can be sent.
  @objc func cloudAvailabilityChanged(_ notification: Notification) {
    LOG(.debug, "iCloudMonitor: Cloud availability changed to : \(isCloudAvailable())")
    isCloudAvailable() ? startQuery() : stopQuery()
  }

  // MARK: - MetadataQuery
  func subscribeOnMetadataQueryNotifications() {
    NotificationCenter.default.addObserver(self, selector: #selector(queryDidFinishGathering(_:)), name: NSNotification.Name.NSMetadataQueryDidFinishGathering, object: nil)
    NotificationCenter.default.addObserver(self, selector: #selector(queryDidUpdate(_:)), name: NSNotification.Name.NSMetadataQueryDidUpdate, object: nil)
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

  @objc func queryDidFinishGathering(_ notification: Notification) {
    guard isCloudAvailable(), let ubiquitousDocumentsDirectory else { return }
    metadataQuery?.disableUpdates()
    LOG(.debug, "Query did finish gathering")
    do {
      let contents = try Self.getContentsFromNotification(notification)
      let trashedContents = try Self.getTrashedContentsFromTrashDirectory(fileManager: fileManager, ubiquitousDocumentsDirectory: ubiquitousDocumentsDirectory)
      delegate?.didFinishGathering(contents: contents + trashedContents)
    } catch {
      delegate?.didReceiveCloudMonitorError(error)
    }
    metadataQuery?.enableUpdates()
  }

  @objc func queryDidUpdate(_ notification: Notification) {
    guard isCloudAvailable() else { return }
    metadataQuery?.disableUpdates()
    LOG(.debug, "Query did update")
    do {
      let contents = try Self.getContentsFromNotification(notification)
      let trashedContents = try Self.getTrashContentsFromNotification(notification)
      delegate?.didUpdate(contents: contents + trashedContents)
    } catch {
      delegate?.didReceiveCloudMonitorError(error)
    }
    metadataQuery?.enableUpdates()
  }
}

fileprivate extension Array where Element == NSMetadataItem {
  var shortDebugDescription: String {
    map { $0.value(forAttribute: NSMetadataItemFSNameKey) as! String }.joined(separator: "\n")
  }
}
