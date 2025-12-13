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

private let kBookmarksDirectoryName = "bookmarks"
private let kICloudSynchronizationDidChangeEnabledStateNotificationName = "iCloudSynchronizationDidChangeEnabledStateNotification"
private let kUDDidFinishInitialCloudSynchronization = "kUDDidFinishInitialCloudSynchronization"

final class SynchronizationManagerState: NSObject {
  let isAvailable: Bool
  let isOn: Bool
  let error: NSError?

  init(isAvailable: Bool, isOn: Bool, error: NSError?) {
    self.isAvailable = isAvailable
    self.isOn = isOn
    self.error = error
  }
}

@objcMembers
final class iCloudSynchronizationManager: NSObject {

  fileprivate struct Observation {
    weak var observer: AnyObject?
    var onSynchronizationStateDidChangeHandler: ((SynchronizationManagerState) -> Void)?
  }

  let fileManager: FileManager
  private let localDirectoryMonitor: LocalDirectoryMonitor
  private let cloudDirectoryMonitor: CloudDirectoryMonitor
  private let settings: Settings.Type
  private let bookmarksManager: BookmarksManager
  private var synchronizationStateManager: SynchronizationStateResolver
  private var fileWriter: SynchronizationFileWriter?
  private var observers = [ObjectIdentifier: iCloudSynchronizationManager.Observation]()
  private var synchronizationError: Error? {
    didSet { notifyObserversOnSynchronizationError(synchronizationError) }
  }

  static private var isInitialSynchronization: Bool {
    get {
      !UserDefaults.standard.bool(forKey: kUDDidFinishInitialCloudSynchronization)
    }
    set {
      UserDefaults.standard.set(!newValue, forKey: kUDDidFinishInitialCloudSynchronization)
    }
  }

  static let shared: iCloudSynchronizationManager = {
    let fileManager = FileManager.default
    let fileType = FileType.kml

    let cloudDirectoryMonitor = iCloudDocumentsMonitor(fileManager: fileManager, fileType: fileType)
    let synchronizationStateManager = iCloudSynchronizationStateResolver(isInitialSynchronization: isInitialSynchronization)
    do {
      let localDirectoryMonitor = try FileSystemDispatchSourceMonitor(directory: fileManager.bookmarksDirectoryUrl)
      let clodStorageManager = iCloudSynchronizationManager(fileManager: fileManager,
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
       synchronizationStateManager: SynchronizationStateResolver) {
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
    cloudDirectoryMonitor.delegate = self
    localDirectoryMonitor.delegate = self
    startSynchronization()
  }
}

// MARK: - Private
private extension iCloudSynchronizationManager {
  // MARK: - Synchronization Lifecycle
  func startSynchronization() {
    switch cloudDirectoryMonitor.state {
    case .started:
      LOG(.debug, "SYNC: Synchronization is already started")
      return
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
              LOG(.info, "SYNC: Start synchronization")
              self.fileWriter = SynchronizationFileWriter(fileManager: self.fileManager,
                                                          localDirectoryUrl: localDirectoryUrl,
                                                          cloudDirectoryUrl: cloudDirectoryUrl)
            }
          }
        }
      }
    }
  }

  func stopSynchronization(withError error: Error? = nil) {
    LOG(.info, "SYNC: Stop synchronization")
    localDirectoryMonitor.stop()
    cloudDirectoryMonitor.stop()
    fileWriter = nil
    synchronizationStateManager.resetState()
    
    guard let error else { return }
    settings.setICLoudSynchronizationEnabled(false)
    synchronizationError = error
    DispatchQueue.main.async {
      MWMAlertViewController.activeAlert().presentBugReportAlert(withTitle: L("icloud_synchronization_error_alert_title"))
    }
  }

  // MARK: - App Lifecycle
  func subscribeToSettingsNotifications() {
    NotificationCenter.default.addObserver(self, selector: #selector(didChangeEnabledState), name: NSNotification.iCloudSynchronizationDidChangeEnabledState, object: nil)
  }

  @objc func didChangeEnabledState() {
    if settings.iCLoudSynchronizationEnabled() {
      Self.isInitialSynchronization = true
      synchronizationStateManager.setInitialSynchronization(true)
      startSynchronization()
    } else {
      stopSynchronization()
    }
  }
}

// MARK: - iCloudStorageManger + LocalDirectoryMonitorDelegate
extension iCloudSynchronizationManager: LocalDirectoryMonitorDelegate {
  func didFinishGathering(_ contents: LocalContents) {
    let events = synchronizationStateManager.resolveEvent(.didFinishGatheringLocalContents(contents))
    processEvents(events)
  }

  func didUpdate(_ contents: LocalContents, _ update: LocalContentsUpdate) {
    let events = synchronizationStateManager.resolveEvent(.didUpdateLocalContents(contents: contents, update: update))
    processEvents(events)
  }

  func didReceiveLocalMonitorError(_ error: Error) {
    processError(error)
  }
}

// MARK: - iCloudStorageManger + CloudDirectoryMonitorDelegate
extension iCloudSynchronizationManager: CloudDirectoryMonitorDelegate {
  func didFinishGathering(_ contents: CloudContents) {
    let events = synchronizationStateManager.resolveEvent(.didFinishGatheringCloudContents(contents))
    processEvents(events)
  }

  func didUpdate(_ contents: CloudContents, _ update: CloudContentsUpdate) {
    let events = synchronizationStateManager.resolveEvent(.didUpdateCloudContents(contents: contents, update: update))
    processEvents(events)
  }

  func didReceiveCloudMonitorError(_ error: Error) {
    processError(error)
  }
}

// MARK: - Private methods
private extension iCloudSynchronizationManager {
  func processEvents(_ events: [OutgoingSynchronizationEvent]) {
    guard !events.isEmpty else {
      synchronizationError = nil
      return
    }
    events.forEach { [weak self] event in
      guard let self, let fileWriter else { return }
      fileWriter.processEvent(event, completion: writingResultHandler(for: event))
    }
  }

  func writingResultHandler(for event: OutgoingSynchronizationEvent) -> WritingResultCompletionHandler {
    return { [weak self] result in
      guard let self else { return }
      switch result {
      case .success:
        if case .didFinishInitialSynchronization = event {
          Self.isInitialSynchronization = false
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
        LOG(.warning, "SYNC: Warning: \(syncError.localizedDescription)")
        synchronizationError = syncError
      case .iCloudIsNotAvailable:
        LOG(.warning, "SYNC: Warning: \(error.localizedDescription)")
        stopSynchronization()
      case .failedToOpenLocalDirectoryFileDescriptor,
           .failedToRetrieveLocalDirectoryContent,
           .containerNotFound,
           .failedToCreateMetadataItem,
           .failedToRetrieveMetadataQueryContent:
        LOG(.error, "SYNC: Error: \(error.localizedDescription)")
        stopSynchronization(withError: error)
      }
    default:
      LOG(.error, "SYNC: System error: \(error.localizedDescription)")
      stopSynchronization(withError: error)
    }
  }
}

// MARK: - Observation
protocol SynchronizationStateObservation {
  func addObserver(_ observer: AnyObject, synchronizationStateDidChangeHandler: @escaping (SynchronizationManagerState) -> Void)
  func removeObserver(_ observer: AnyObject)
}

extension iCloudSynchronizationManager {
  func addObserver(_ observer: AnyObject, synchronizationStateDidChangeHandler: @escaping (SynchronizationManagerState) -> Void) {
    let id = ObjectIdentifier(observer)
    observers[id] = Observation(observer: observer, onSynchronizationStateDidChangeHandler: synchronizationStateDidChangeHandler)
    notifyObserversOnSynchronizationError(synchronizationError)
  }

  func removeObserver(_ observer: AnyObject) {
    let id = ObjectIdentifier(observer)
    observers.removeValue(forKey: id)
  }

  private func notifyObserversOnSynchronizationError(_ error: Error?) {
    let state = SynchronizationManagerState(isAvailable: cloudDirectoryMonitor.isCloudAvailable(),
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
}

// MARK: - Notification + iCloudSynchronizationDidChangeEnabledState
extension Notification.Name {
  static let iCloudSynchronizationDidChangeEnabledStateNotification = Notification.Name(kICloudSynchronizationDidChangeEnabledStateNotificationName)
}

@objc extension NSNotification {
  public static let iCloudSynchronizationDidChangeEnabledState = Notification.Name.iCloudSynchronizationDidChangeEnabledStateNotification
}

// MARK: - Dictionary + RemoveUnreachable
private extension Dictionary where Key == ObjectIdentifier, Value == iCloudSynchronizationManager.Observation {
  mutating func removeUnreachable() -> Self {
    for (id, observation) in self {
      if observation.observer == nil {
        removeValue(forKey: id)
      }
    }
    return self
  }
}
