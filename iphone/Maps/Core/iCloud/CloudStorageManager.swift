enum VoidResult {
  case success
  case failure(Error)
}

enum WritingResult {
  case success
  case reloadCategoriesAtURLs([URL])
  case deleteCategoriesAtURLs([URL])
  case failure(Error)
}

typealias VoidResultCompletionHandler = (VoidResult) -> Void
typealias WritingResultCompletionHandler = (WritingResult) -> Void

let kTrashDirectoryName = ".Trash"
private let kBookmarksDirectoryName = "bookmarks"
private let kICloudSynchronizationDidChangeEnabledStateNotificationName = "iCloudSynchronizationDidChangeEnabledStateNotification"
private let kUDDidFinishInitialCloudSynchronization = "kUDDidFinishInitialCloudSynchronization"

final class CloudStorageSynchronizationState: NSObject {
  let isAvailable: Bool
  let isOn: Bool
  let error: NSError?

  init(isAvailable: Bool, isOn: Bool, error: NSError?) {
    self.isAvailable = isAvailable
    self.isOn = isOn
    self.error = error
  }
}

@objc @objcMembers final class CloudStorageManager: NSObject {

  fileprivate struct Observation {
    weak var observer: AnyObject?
    var onSynchronizationStateDidChangeHandler: ((CloudStorageSynchronizationState) -> Void)?
  }

  let fileManager: FileManager
  private let localDirectoryMonitor: LocalDirectoryMonitor
  private let cloudDirectoryMonitor: CloudDirectoryMonitor
  private let settings: Settings.Type
  private let bookmarksManager: BookmarksManager
  private let synchronizationStateManager: SynchronizationStateManager
  private var fileWriter: SynchronizationFileWriter?
  private var observers = [ObjectIdentifier: CloudStorageManager.Observation]()
  private var synchronizationError: Error? {
    didSet { notifyObserversOnSynchronizationError(synchronizationError) }
  }

  static private var isInitialSynchronization: Bool {
    return !UserDefaults.standard.bool(forKey: kUDDidFinishInitialCloudSynchronization)
  }

  static let shared: CloudStorageManager = {
    let fileManager = FileManager.default
    let fileType = FileType.kml
    let cloudDirectoryMonitor = iCloudDocumentsDirectoryMonitor(fileManager: fileManager, fileType: fileType)
    let synchronizationStateManager = DefaultSynchronizationStateManager(isInitialSynchronization: CloudStorageManager.isInitialSynchronization)
    do {
      let localDirectoryMonitor = try DefaultLocalDirectoryMonitor(fileManager: fileManager, directory: fileManager.bookmarksDirectoryUrl, fileType: fileType)
      let clodStorageManager = try CloudStorageManager(fileManager: fileManager,
                                                       settings: Settings.self,
                                                       bookmarksManager: BookmarksManager.shared(),
                                                       cloudDirectoryMonitor: cloudDirectoryMonitor,
                                                       localDirectoryMonitor: localDirectoryMonitor,
                                                       synchronizationStateManager: synchronizationStateManager)
      return clodStorageManager
    } catch {
      fatalError("Failed to create shared iCloud storage manager with error: \(error)")
    }
  }()

  // MARK: - Initialization
  init(fileManager: FileManager,
       settings: Settings.Type,
       bookmarksManager: BookmarksManager,
       cloudDirectoryMonitor: CloudDirectoryMonitor,
       localDirectoryMonitor: LocalDirectoryMonitor,
       synchronizationStateManager: SynchronizationStateManager) throws {
    guard fileManager === cloudDirectoryMonitor.fileManager, fileManager === localDirectoryMonitor.fileManager else {
      throw NSError(domain: "CloudStorageManger", code: 0, userInfo: [NSLocalizedDescriptionKey: "File managers should be the same."])
    }
    self.fileManager = fileManager
    self.settings = settings
    self.bookmarksManager = bookmarksManager
    self.cloudDirectoryMonitor = cloudDirectoryMonitor
    self.localDirectoryMonitor = localDirectoryMonitor
    self.synchronizationStateManager = synchronizationStateManager
    super.init()
  }

  // MARK: - Public
  @objc func start() {
    subscribeToSettingsNotifications()
    subscribeToApplicationLifecycleNotifications()
    cloudDirectoryMonitor.delegate = self
    localDirectoryMonitor.delegate = self
  }
}

// MARK: - Private
private extension CloudStorageManager {
  // MARK: - Synchronization Lifecycle
  func startSynchronization() {
    LOG(.info, "Start synchronization...")
    switch cloudDirectoryMonitor.state {
    case .started:
      LOG(.debug, "Synchronization is already started")
      return
    case .paused:
      resumeSynchronization()
    case .stopped:
      cloudDirectoryMonitor.start { [weak self] result in
        guard let self else { return }
        switch result {
        case .failure(let error):
          self.processError(error)
        case .success(let cloudDirectoryUrl):
          self.localDirectoryMonitor.start { result in
            switch result {
            case .failure(let error):
              self.processError(error)
            case .success(let localDirectoryUrl):
              self.fileWriter = SynchronizationFileWriter(fileManager: self.fileManager,
                                                          localDirectoryUrl: localDirectoryUrl,
                                                          cloudDirectoryUrl: cloudDirectoryUrl)
              LOG(.debug, "Synchronization is started successfully")
            }
          }
        }
      }
    }
  }

  func stopSynchronization(withError error: Error? = nil) {
    LOG(.info, "Stop synchronization")
    localDirectoryMonitor.stop()
    cloudDirectoryMonitor.stop()
    fileWriter = nil
    synchronizationStateManager.resetState()
    
    guard let error else { return }
    settings.setICLoudSynchronizationEnabled(false)
    synchronizationError = error
    MWMAlertViewController.activeAlert().presentBugReportAlert(withTitle: L("icloud_synchronization_error_alert_title"))
  }

  func pauseSynchronization() {
    LOG(.info, "Pause synchronization")
    localDirectoryMonitor.pause()
    cloudDirectoryMonitor.pause()
  }

  func resumeSynchronization() {
    LOG(.info, "Resume synchronization")
    localDirectoryMonitor.resume()
    cloudDirectoryMonitor.resume()
  }

  // MARK: - App Lifecycle
  func subscribeToApplicationLifecycleNotifications() {
    NotificationCenter.default.addObserver(self, selector: #selector(appWillEnterForeground), name: UIApplication.didBecomeActiveNotification, object: nil)
    NotificationCenter.default.addObserver(self, selector: #selector(appDidEnterBackground), name: UIApplication.didEnterBackgroundNotification, object: nil)
  }

  func unsubscribeFromApplicationLifecycleNotifications() {
    NotificationCenter.default.removeObserver(self, name: UIApplication.didBecomeActiveNotification, object: nil)
    NotificationCenter.default.removeObserver(self, name: UIApplication.didEnterBackgroundNotification, object: nil)
  }

  func subscribeToSettingsNotifications() {
    NotificationCenter.default.addObserver(self, selector: #selector(didChangeEnabledState), name: NSNotification.iCloudSynchronizationDidChangeEnabledState, object: nil)
  }

  @objc func appWillEnterForeground() {
    guard settings.iCLoudSynchronizationEnabled() else { return }
    startSynchronization()
  }

  @objc func appDidEnterBackground() {
    guard settings.iCLoudSynchronizationEnabled() else { return }
    pauseSynchronization()
  }

  @objc func didChangeEnabledState() {
    settings.iCLoudSynchronizationEnabled() ? startSynchronization() : stopSynchronization()
  }
}

// MARK: - iCloudStorageManger + LocalDirectoryMonitorDelegate
extension CloudStorageManager: LocalDirectoryMonitorDelegate {
  func didFinishGathering(contents: LocalContents) {
    let events = synchronizationStateManager.resolveEvent(.didFinishGatheringLocalContents(contents))
    processEvents(events)
  }

  func didUpdate(contents: LocalContents) {
    let events = synchronizationStateManager.resolveEvent(.didUpdateLocalContents(contents))
    processEvents(events)
  }

  func didReceiveLocalMonitorError(_ error: Error) {
    processError(error)
  }
}

// MARK: - iCloudStorageManger + CloudDirectoryMonitorDelegate
extension CloudStorageManager: CloudDirectoryMonitorDelegate {
  func didFinishGathering(contents: CloudContents) {
    let events = synchronizationStateManager.resolveEvent(.didFinishGatheringCloudContents(contents))
    processEvents(events)
  }

  func didUpdate(contents: CloudContents) {
    let events = synchronizationStateManager.resolveEvent(.didUpdateCloudContents(contents))
    processEvents(events)
  }

  func didReceiveCloudMonitorError(_ error: Error) {
    processError(error)
  }
}

// MARK: - Private methods
private extension CloudStorageManager {
  func processEvents(_ events: [OutgoingEvent]) {
    guard !events.isEmpty else {
      synchronizationError = nil
      return
    }
    events.forEach { [weak self] event in
      guard let self, let fileWriter else { return }
      fileWriter.processEvent(event, completion: writingResultHandler(for: event))
    }
  }

  func writingResultHandler(for event: OutgoingEvent) -> WritingResultCompletionHandler {
    return { [weak self] result in
      guard let self else { return }
      switch result {
      case .success:
        // Mark that initial synchronization is finished.
        if case .didFinishInitialSynchronization = event {
          UserDefaults.standard.set(true, forKey: kUDDidFinishInitialCloudSynchronization)
        }
      case .reloadCategoriesAtURLs(let urls):
        urls.forEach { self.bookmarksManager.reloadCategory(atFilePath: $0.path) }
      case .deleteCategoriesAtURLs(let urls):
        urls.forEach { self.bookmarksManager.deleteCategory(atFilePath: $0.path) }
      case .failure(let error):
        self.processError(error)
      }
    }
  }

  // MARK: - Error handling
  func processError(_ error: Error) {
    switch error {
    case let syncError as SynchronizationError:
      switch syncError {
      case .fileUnavailable,
           .fileNotUploadedDueToQuota,
           .ubiquityServerNotAvailable:
        LOG(.warning, "Synchronization Warning: \(syncError.localizedDescription)")
        synchronizationError = syncError
      case .iCloudIsNotAvailable:
        LOG(.warning, "Synchronization Warning: \(error.localizedDescription)")
        stopSynchronization()
      case .failedToOpenLocalDirectoryFileDescriptor,
           .failedToRetrieveLocalDirectoryContent,
           .containerNotFound,
           .failedToCreateMetadataItem,
           .failedToRetrieveMetadataQueryContent:
        LOG(.error, "Synchronization Error: \(error.localizedDescription)")
        stopSynchronization(withError: error)
      }
    default:
      LOG(.error, "System Error: \(error.localizedDescription)")
      stopSynchronization(withError: error)
    }
  }
}

// MARK: - CloudStorageManger Observing
extension CloudStorageManager {
  func addObserver(_ observer: AnyObject, synchronizationStateDidChangeHandler: @escaping (CloudStorageSynchronizationState) -> Void) {
    let id = ObjectIdentifier(observer)
    observers[id] = Observation(observer: observer, onSynchronizationStateDidChangeHandler: synchronizationStateDidChangeHandler)
    notifyObserversOnSynchronizationError(synchronizationError)
  }

  func removeObserver(_ observer: AnyObject) {
    let id = ObjectIdentifier(observer)
    observers.removeValue(forKey: id)
  }

  private func notifyObserversOnSynchronizationError(_ error: Error?) {
    let state = CloudStorageSynchronizationState(isAvailable: cloudDirectoryMonitor.isCloudAvailable(),
                                     isOn: settings.iCLoudSynchronizationEnabled(),
                                     error: error as? NSError)
    observers.removeUnreachable().forEach { _, observable in
      DispatchQueue.main.async {
        observable.onSynchronizationStateDidChangeHandler?(state)
      }
    }
  }
}

// MARK: - FileManager + Directories
extension FileManager {
  var bookmarksDirectoryUrl: URL {
    urls(for: .documentDirectory, in: .userDomainMask).first!.appendingPathComponent(kBookmarksDirectoryName, isDirectory: true)
  }

  func trashDirectoryUrl(for baseDirectoryUrl: URL) throws -> URL {
    let trashDirectory = baseDirectoryUrl.appendingPathComponent(kTrashDirectoryName, isDirectory: true)
    if !fileExists(atPath: trashDirectory.path) {
      try createDirectory(at: trashDirectory, withIntermediateDirectories: true)
    }
    return trashDirectory
  }
}

// MARK: - Notification + iCloudSynchronizationDidChangeEnabledState
extension Notification.Name {
  static let iCloudSynchronizationDidChangeEnabledStateNotification = Notification.Name(kICloudSynchronizationDidChangeEnabledStateNotificationName)
}

@objc extension NSNotification {
  public static let iCloudSynchronizationDidChangeEnabledState = Notification.Name.iCloudSynchronizationDidChangeEnabledStateNotification
}

// MARK: - Dictionary + RemoveUnreachable
private extension Dictionary where Key == ObjectIdentifier, Value == CloudStorageManager.Observation {
  mutating func removeUnreachable() -> Self {
    for (id, observation) in self {
      if observation.observer == nil {
        removeValue(forKey: id)
      }
    }
    return self
  }
}
