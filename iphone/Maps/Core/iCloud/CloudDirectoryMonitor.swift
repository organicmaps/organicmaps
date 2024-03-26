private let kUDCloudIdentityKey = "com.apple.organicmaps.UbiquityIdentityToken"

protocol CloudDirectoryMonitorDelegate : AnyObject {
  func didFinishGathering(contents: CloudContents)
  func didUpdate(contents: CloudContents)
}

final class CloudDirectoryMonitor: NSObject {

  private static let sharedContainerIdentifier: String = {
    var identifier = "iCloud.app.organicmaps"
    #if DEBUG
    identifier.append(".debug")
    #endif
    return identifier
  }()

  static let `default` = CloudDirectoryMonitor(cloudContainerIdentifier: CloudDirectoryMonitor.sharedContainerIdentifier)

  private let metadataQuery = NSMetadataQuery()
  private var containerIdentifier: String
  private var ubiquitousDocumentsDirectory: URL?

  weak var delegate: CloudDirectoryMonitorDelegate?

  init(cloudContainerIdentifier: String = CloudDirectoryMonitor.sharedContainerIdentifier) {
    self.containerIdentifier = cloudContainerIdentifier
    super.init()

    setupMetadataQuery()
    subscribeToCloudAvailabilityNotifications()
    fetchUbiquityDirectoryUrl()
  }

  // MARK: - Public
  var isStarted: Bool { return metadataQuery.isStarted }

  func start(completion: VoidResultCompletionHandler? = nil) {
    guard cloudIsAvailable() else {
      completion?(.failure(CloudSynchronizationError.iCloudIsNotAvailable))
      return
    }
    fetchUbiquityDirectoryUrl { [weak self] result in
      guard let self else { return }
      switch result {
      case .failure(let error):
        completion?(.failure(error))
      case .success:
        self.startQuery()
        completion?(.success)
      }
    }
  }

  func stop() {
    stopQuery()
  }

  func resume() {
    metadataQuery.enableUpdates()
  }

  func pause() {
    metadataQuery.disableUpdates()
  }

  func fetchUbiquityDirectoryUrl(completion: ((Result<URL, CloudSynchronizationError>) -> Void)? = nil) {
    if let ubiquitousDocumentsDirectory {
      completion?(.success(ubiquitousDocumentsDirectory))
      return
    }
    DispatchQueue.global().async {
      guard let containerUrl = FileManager.default.url(forUbiquityContainerIdentifier: self.containerIdentifier) else {
        LOG(.error, "Failed to retrieve container's URL for:\(self.containerIdentifier)")
        completion?(.failure(.containerNotFound))
        return
      }
      let documentsContainerUrl = containerUrl.appendingPathComponent(kDocumentsDirectoryName)
      self.ubiquitousDocumentsDirectory = documentsContainerUrl
      completion?(.success(documentsContainerUrl))
    }
  }
}

// MARK: - Private
private extension CloudDirectoryMonitor {
  func cloudIsAvailable() -> Bool {
    let cloudToken = FileManager.default.ubiquityIdentityToken
    guard let cloudToken else {
      UserDefaults.standard.removeObject(forKey: kUDCloudIdentityKey)
      LOG(.debug, "Cloud is not available. Cloud token is nil.")
      return false
    }
    do {
      let data = try NSKeyedArchiver.archivedData(withRootObject: cloudToken, requiringSecureCoding: true)
      UserDefaults.standard.set(data, forKey: kUDCloudIdentityKey)
      LOG(.debug, "Cloud is available.")
      return true
    } catch {
      UserDefaults.standard.removeObject(forKey: kUDCloudIdentityKey)
      LOG(.error, "Failed to archive cloud token: \(error)")
      return false
    }
  }

  func subscribeToCloudAvailabilityNotifications() {
    NotificationCenter.default.addObserver(self, selector: #selector(cloudAvailabilityChanged(_:)), name: .NSUbiquityIdentityDidChange, object: nil)
  }

  // FIXME: - Actually this notification was never called. If user disable the iCloud for the curren app during the active state the app will be relaunched. Needs to investigate additional cases when this notification can be sent.
  @objc func cloudAvailabilityChanged(_ notification: Notification) {
    LOG(.debug, "Cloud availability changed to : \(cloudIsAvailable())")
    cloudIsAvailable() ? startQuery() : stopQuery()
  }

  // MARK: - MetadataQuery
  private func setupMetadataQuery() {
    metadataQuery.notificationBatchingInterval = 1
    metadataQuery.searchScopes = [NSMetadataQueryUbiquitousDocumentsScope]
    metadataQuery.predicate = NSPredicate(format: "%K LIKE %@", NSMetadataItemFSNameKey, "*.\(kFileExtensionKML)")
    metadataQuery.sortDescriptors = [NSSortDescriptor(key: NSMetadataItemFSNameKey, ascending: true)]

    NotificationCenter.default.addObserver(self, selector: #selector(queryDidFinishGathering(_:)), name: NSNotification.Name.NSMetadataQueryDidFinishGathering, object: nil)
    NotificationCenter.default.addObserver(self, selector: #selector(queryDidUpdate(_:)), name: NSNotification.Name.NSMetadataQueryDidUpdate, object: nil)
  }

  func startQuery() {
    LOG(.info, "Start quering metadata.")
    guard !metadataQuery.isStarted else { return }
    metadataQuery.start()
  }

  func stopQuery() {
    LOG(.info, "Stop quering metadata.")
    metadataQuery.stop()
  }

  @objc func queryDidFinishGathering(_ notification: Notification) {
    guard cloudIsAvailable(), notification.object as? NSMetadataQuery === metadataQuery else { return }
    let newContent = getContentFromMetadataQuery(metadataQuery)
    delegate?.didFinishGathering(contents: newContent)
  }

  @objc func queryDidUpdate(_ notification: Notification) {
    guard cloudIsAvailable(), notification.object as? NSMetadataQuery === metadataQuery else { return }
    let newContent = getContentFromMetadataQuery(metadataQuery)
    delegate?.didUpdate(contents: newContent)
  }

  private func getContentFromMetadataQuery(_ metadataQuery: NSMetadataQuery) -> CloudContents {
    var content = CloudContents()
    metadataQuery.enumerateResults { result, _, _ in
      guard let metadataItem = result as? NSMetadataItem else { return }
      do {
        let cloudMetadataItem = try CloudMetadataItem(metadataItem: metadataItem)
        content.add(cloudMetadataItem)
      } catch {
        LOG(.error, "Failed to create CloudMetadataItem: \(error)")
      }
    }
    return content
  }
}
