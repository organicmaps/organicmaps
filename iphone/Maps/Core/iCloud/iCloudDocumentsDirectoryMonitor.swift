protocol CloudDirectoryMonitor: DirectoryMonitor {
  var fileManager: FileManager { get }
  var ubiquitousDocumentsDirectory: URL? { get }
  var delegate: CloudDirectoryMonitorDelegate? { get set }

  func fetchUbiquityDirectoryUrl(completion: ((Result<URL, Error>) -> Void)?)
  func isCloudAvailable() -> Bool
}

protocol CloudDirectoryMonitorDelegate : AnyObject {
  func didFinishGathering(contents: CloudContentsMetadata)
  func didUpdate(contents: CloudContentsMetadata)
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
  private let fileTypes: [FileType] // TODO: Should be removed when the nested directory support will be implemented
  private(set) var metadataQuery: NSMetadataQuery?
  private(set) var ubiquitousDocumentsDirectory: URL?

  // MARK: - Public properties
  private(set) var state: DirectoryMonitorState = .stopped
  weak var delegate: CloudDirectoryMonitorDelegate?

  init(fileManager: FileManager = .default, cloudContainerIdentifier: String = iCloudDocumentsDirectoryMonitor.sharedContainerIdentifier, fileType: FileType) {
    self.fileManager = fileManager
    self.containerIdentifier = cloudContainerIdentifier
    self.fileTypes = [fileType, .deleted]
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
        LOG(.debug, "iCloudMonitor: Start")
        self.startQuery()
        self.state = .started
        completion?(.success(url))
      }
    }
  }

  func stop() {
    guard state != .stopped else { return }
    LOG(.debug, "iCloudMonitor: Stop")
    stopQuery()
    state = .stopped
  }

  func resume() {
    guard state != .started else { return }
    LOG(.debug, "iCloudMonitor: Resume")
    metadataQuery?.enableUpdates()
    state = .started
  }

  func pause() {
    guard state != .paused else { return }
    LOG(.debug, "iCloudMonitor: Pause")
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
        LOG(.debug, "iCloudMonitor: Failed to retrieve container's URL for:\(self.containerIdentifier)")
        completion?(.failure(SynchronizationError.containerNotFound))
        return
      }
      let documentsContainerUrl = containerUrl.appendingPathComponent(kDocumentsDirectoryName)
      if !self.fileManager.fileExists(atPath: documentsContainerUrl.path) {
        LOG(.debug, "iCloudMonitor: Creating directory at path: \(documentsContainerUrl.path)")
        do {
          try self.fileManager.createDirectory(at: documentsContainerUrl, withIntermediateDirectories: true)
        } catch {
          completion?(.failure(SynchronizationError.containerNotFound))
        }
      }
      LOG(.debug, "iCloudMonitor: Ubiquity directory URL: \(documentsContainerUrl)")
      self.ubiquitousDocumentsDirectory = documentsContainerUrl
      completion?(.success(documentsContainerUrl))
    }
  }

  func isCloudAvailable() -> Bool {
    let cloudToken = fileManager.ubiquityIdentityToken
    guard let cloudToken else {
      UserDefaults.standard.removeObject(forKey: kUDCloudIdentityKey)
      LOG(.debug, "iCloudMonitor: Cloud is not available. Cloud token is nil.")
      return false
    }
    do {
      let data = try NSKeyedArchiver.archivedData(withRootObject: cloudToken, requiringSecureCoding: true)
      UserDefaults.standard.set(data, forKey: kUDCloudIdentityKey)
      return true
    } catch {
      UserDefaults.standard.removeObject(forKey: kUDCloudIdentityKey)
      LOG(.debug, "iCloudMonitor: Failed to archive cloud token: \(error)")
      return false
    }
  }

  class func buildMetadataQuery(for fileTypes: [FileType]) -> NSMetadataQuery {
    let metadataQuery = NSMetadataQuery()
    metadataQuery.notificationBatchingInterval = 1
    metadataQuery.searchScopes = [NSMetadataQueryUbiquitousDocumentsScope]

    let predicates = fileTypes.map { fileType in
      NSPredicate(format: "%K LIKE %@", NSMetadataItemFSNameKey, "*.\(fileType.fileExtension)")
    }

    let compoundPredicate = NSCompoundPredicate(orPredicateWithSubpredicates: predicates)
    metadataQuery.predicate = compoundPredicate
    metadataQuery.sortDescriptors = [NSSortDescriptor(key: NSMetadataItemFSNameKey, ascending: true)]
    return metadataQuery
  }

  static func getContentsFromNotification(_ notification: Notification, _ onError: (Error) -> Void) -> CloudContentsMetadata {
    guard let metadataQuery = notification.object as? NSMetadataQuery,
          let metadataItems = metadataQuery.results as? [NSMetadataItem] else {
      return []
    }

    let cloudMetadataItems = CloudContentsMetadata(metadataItems.compactMap { item in
      do {
        return try CloudMetadataItem(metadataItem: item)
      } catch {
        onError(error)
        return nil
      }
    })
    return cloudMetadataItems
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
    metadataQuery = Self.buildMetadataQuery(for: fileTypes)
    guard let metadataQuery, !metadataQuery.isStarted else { return }
    LOG(.debug, "iCloudMonitor: Start metadata query")
    metadataQuery.start()
  }

  func stopQuery() {
    LOG(.debug, "iCloudMonitor: Stop metadata query")
    metadataQuery?.stop()
    metadataQuery = nil
  }

  @objc func queryDidFinishGathering(_ notification: Notification) {
    guard isCloudAvailable() else { return }
    metadataQuery?.disableUpdates()
    LOG(.debug, "iCloudMonitor: Query did finish gathering")
    let contents = Self.getContentsFromNotification(notification, metadataQueryErrorHandler)
    let removedContents = Self.getTrashedContentsFromTrashDirectory(fileManager: fileManager,
                                                                    ubiquitousDocumentsDirectory: ubiquitousDocumentsDirectory,
                                                                    onError: metadataQueryErrorHandler)
    LOG(.debug, "iCloudMonitor: Cloud contents count: \(contents.count)")
    LOG(.debug, "iCloudMonitor: Trashed contents count: \(removedContents.count)")
    delegate?.didFinishGathering(contents: contents + removedContents)
    metadataQuery?.enableUpdates()
  }

  @objc func queryDidUpdate(_ notification: Notification) {
    guard isCloudAvailable() else { return }
    metadataQuery?.disableUpdates()
    LOG(.debug, "iCloudMonitor: Query did update")
    let contents = Self.getContentsFromNotification(notification, metadataQueryErrorHandler)
    let removedContents = Self.getRemovedContentsFromNotification(notification, onError: metadataQueryErrorHandler)
    LOG(.debug, "iCloudMonitor: Cloud contents count: \(contents.count)")
    LOG(.debug, "iCloudMonitor: Trashed contents count: \(removedContents.count)")
    delegate?.didUpdate(contents: contents + removedContents)
    metadataQuery?.enableUpdates()
  }

  private var metadataQueryErrorHandler: (Error) -> Void {
    { [weak self] error in
      self?.delegate?.didReceiveCloudMonitorError(error)
    }
  }

  // There are no ways to retrieve the content of iCloud's .Trash directory on the macOS because it uses different file system and place trashed content in the /Users/<user_name>/.Trash which cannot be observed without access.
  // When we get a new notification and retrieve the metadata from the object the actual list of items in iOS contains both current and deleted files (which is in .Trash/ directory now) but on macOS we only have absence of the file. So there are no way to get list of deleted items on macOS on didFinishGathering state.
  // Due to didUpdate state we can get the list of deleted items on macOS from the userInfo property but cannot get their new url.
  static private func getRemovedContentsFromNotification(_ notification: Notification, onError: (Error) -> Void) -> CloudContentsMetadata {
    guard let removedItems = notification.userInfo?[NSMetadataQueryUpdateRemovedItemsKey] as? [NSMetadataItem] else { return [] }
    return CloudContentsMetadata(removedItems.compactMap { metadataItem in
      do {
        // on macOS deleted file will not be in the ./Trash directory, but it doesn't mean that it is not removed because it is placed in the NSMetadataQueryUpdateRemovedItems array.
        return try CloudMetadataItem(metadataItem: metadataItem, isRemoved: true)
      } catch {
        onError(error)
        return nil
      }
    })
  }

  static private func getTrashedContentsFromTrashDirectory(fileManager: FileManager, ubiquitousDocumentsDirectory: URL?, onError: (Error) -> Void) -> CloudContentsMetadata {
    // There are no ways to retrieve the content of iCloud's .Trash directory on macOS.
    if #available(iOS 14.0, *), ProcessInfo.processInfo.isiOSAppOnMac {
      return []
    }
    // On iOS we can get the list of deleted items from the .Trash directory but only when iCloud is enabled.
    guard let ubiquitousDocumentsDirectory,
          let trashDirectoryUrl = try? fileManager.trashDirectoryUrl(for: ubiquitousDocumentsDirectory),
          let removedItems = try? fileManager.contentsOfDirectory(at: trashDirectoryUrl,
                                                                  includingPropertiesForKeys: [],
                                                                  options: [.skipsPackageDescendants, .skipsSubdirectoryDescendants]) else {
      return []
    }
    let removedCloudMetadataItems = CloudContentsMetadata(removedItems.compactMap { url in
      do {
        return try CloudMetadataItem(fileUrl: url, isRemoved: true)
      } catch {
        onError(error)
        return nil
      }
    })
    return removedCloudMetadataItems
  }
}
