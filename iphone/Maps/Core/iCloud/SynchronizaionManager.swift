enum WritingResult {
  case success
  /// Nothing was written because what the operation assumed is not true anymore. It is not a failure and not a
  /// success either: the file is reconciled again from what is observed next.
  case skipped(String)
  /// The written files have to be loaded by the app. There are two of them when the local version was preserved
  /// under a new name before it was replaced.
  case reloadCategoriesAtURLs([URL])
  /// Deletions are only requested by the writer: they are irreversible and are authorized against the latest
  /// observations, on the queue where the synchronization state lives.
  case deleteCategory(atURL: URL)
  /// The content the file is expected to hold is verified once more, right before it is trashed for good.
  case trashCloudItem(atURL: URL, expecting: Fingerprint)
  case failure(Error)
}

typealias WritingResultCompletionHandler = (WritingResult) -> Void

private let kBookmarksDirectoryName = "bookmarks"
private let kICloudSynchronizationDidChangeEnabledStateNotificationName = "iCloudSynchronizationDidChangeEnabledStateNotification"

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
final class iCloudSynchronizaionManager: NSObject {
  fileprivate struct Observation {
    weak var observer: AnyObject?
    var onSynchronizationStateDidChangeHandler: ((SynchronizationManagerState) -> Void)?
  }

  let fileManager: FileManager
  private let localDirectoryMonitor: LocalDirectoryMonitor
  private let cloudDirectoryMonitor: CloudDirectoryMonitor
  private let settings: Settings.Type
  private let bookmarksManager: BookmarksManager
  private let stateResolver: SynchronizationStateResolver
  private let stateStore: SynchronizedStateStore
  private let fingerprintProvider: FingerprintProvider
  private let clock: ActiveSynchronizationClock
  private var fileWriter: SynchronizationFileWriter?
  private var confirmationTimer: Timer?
  /// Numbers the synchronizations: stopping ends the current one. A write outlives the session that requested
  /// it, and what its result means for the state is only acted on while that session runs: stopping resets the
  /// state, and what is built after it -- from other observations, of another iCloud account after a switch --
  /// knows nothing of that write, so accepting the result would confirm a write that has not happened.
  private var session = 0
  private var observers = [ObjectIdentifier: iCloudSynchronizaionManager.Observation]()
  private var synchronizationError: Error? {
    didSet { notifyObserversOnSynchronizationError(synchronizationError) }
  }

  static let shared: iCloudSynchronizaionManager = {
    let fileManager = FileManager.default
    let fileType = FileType.kml
    let cloudDirectoryMonitor = iCloudDocumentsMonitor(fileManager: fileManager, fileType: fileType)
    let stateStore = FileSynchronizedStateStore()
    let clock = ActiveSynchronizationClock()
    let fingerprintProvider = FileContentFingerprintProvider()
    let stateResolver = iCloudSynchronizationStateResolver(store: stateStore,
                                                           fingerprintProvider: fingerprintProvider,
                                                           clock: clock)
    do {
      let localDirectoryMonitor = try FileSystemDispatchSourceMonitor(fileManager: fileManager, directory: fileManager.bookmarksDirectoryUrl, fileType: fileType)
      return iCloudSynchronizaionManager(fileManager: fileManager,
                                         settings: Settings.self,
                                         bookmarksManager: BookmarksManager.shared(),
                                         cloudDirectoryMonitor: cloudDirectoryMonitor,
                                         localDirectoryMonitor: localDirectoryMonitor,
                                         stateResolver: stateResolver,
                                         stateStore: stateStore,
                                         fingerprintProvider: fingerprintProvider,
                                         clock: clock)
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
       stateResolver: SynchronizationStateResolver,
       stateStore: SynchronizedStateStore,
       fingerprintProvider: FingerprintProvider,
       clock: ActiveSynchronizationClock) {
    self.fileManager = fileManager
    self.settings = settings
    self.bookmarksManager = bookmarksManager
    self.cloudDirectoryMonitor = cloudDirectoryMonitor
    self.localDirectoryMonitor = localDirectoryMonitor
    self.stateResolver = stateResolver
    self.stateStore = stateStore
    self.fingerprintProvider = fingerprintProvider
    self.clock = clock
    super.init()
  }

  // MARK: - Public

  func start() {
    subscribeToSettingsNotifications()
    subscribeToApplicationLifecycleNotifications()
    subscribeToCloudIdentityNotifications()
    cloudDirectoryMonitor.delegate = self
    localDirectoryMonitor.delegate = self
    // The directories did not change, but what is known about their content did -- a file was read in the
    // background, or one that could not be read is worth trying again -- so they are reconciled again.
    fingerprintProvider.onContentsMayBeKnown = { [weak self] in
      guard let self else { return }
      processEvents(stateResolver.resolveEvent(.didUpdateKnownContents))
    }
  }
}

// MARK: - Private

private extension iCloudSynchronizaionManager {
  // MARK: - Synchronization Lifecycle

  func startSynchronization() {
    switch cloudDirectoryMonitor.state {
    case .starting, .started:
      // A start that is in progress installs the file writer itself, once iCloud has answered.
      LOG(.debug, "Synchronization is already started")
      return
    case .paused:
      resumeSynchronization()
    case .stopped:
      /* Files of another iCloud account have nothing in common with the previously synchronized ones. A missing
       token means the account is unknown, not that it is another one -- signed out, iCloud Drive off, container
       not ready yet -- and forgetting the history then would keep both versions of every file that differs. */
      if let cloudIdentity = cloudDirectoryMonitor.cloudIdentity {
        stateStore.resetIfCloudIdentityChanged(cloudIdentity)
      }
      clock.resume()
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
              LOG(.info, "Start synchronization")
              self.fileWriter = SynchronizationFileWriter(fileManager: self.fileManager,
                                                          localDirectoryUrl: localDirectoryUrl,
                                                          cloudDirectoryUrl: cloudDirectoryUrl)
            }
          }
        }
      }
    }
  }

  /** Synchronization can be stopped by a runtime error, but the user's preference is only changed by the user:
   turning it off silently would leave the app unsynchronized until someone notices. */
  func stopSynchronization(withError error: Error? = nil) {
    LOG(.info, "Stop synchronization")
    localDirectoryMonitor.stop()
    cloudDirectoryMonitor.stop()
    cancelConfirmation()
    clock.pause()
    session += 1
    fileWriter = nil
    stateResolver.resetState()
    // Observers are told in both cases: an error that stopped the engine, or that nothing is wrong anymore.
    synchronizationError = error

    guard error != nil else { return }
    MWMAlertViewController.activeAlert().presentBugReportAlert(withTitle: L("icloud_synchronization_error_alert_title"))
  }

  func pauseSynchronization() {
    LOG(.info, "Pause synchronization")
    localDirectoryMonitor.pause()
    cloudDirectoryMonitor.pause()
    cancelConfirmation()
    clock.pause()
  }

  func resumeSynchronization() {
    LOG(.info, "Resume synchronization")
    localDirectoryMonitor.resume()
    cloudDirectoryMonitor.resume()
    clock.resume()
    // Nothing was observed while the app was in the background: everything must be observed anew.
    refreshContents()
  }

  /// Asks both directories to publish their content again. A file that stays missing is not reported by iCloud,
  /// so a fresh look is the only way to confirm that it is really gone.
  func refreshContents() {
    localDirectoryMonitor.refresh()
    cloudDirectoryMonitor.refresh()
  }

  func scheduleConfirmation() {
    guard confirmationTimer == nil else { return }
    confirmationTimer = Timer.scheduledTimer(withTimeInterval: iCloudSynchronizationStateResolver.Constants.absenceConfirmationInterval, repeats: false) { [weak self] _ in
      self?.confirmationTimer = nil
      self?.refreshContents()
    }
  }

  func cancelConfirmation() {
    confirmationTimer?.invalidate()
    confirmationTimer = nil
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

  func subscribeToCloudIdentityNotifications() {
    NotificationCenter.default.addObserver(self, selector: #selector(didChangeCloudIdentity), name: .NSUbiquityIdentityDidChange, object: nil)
  }

  /// The user signed in or out while the app was running: everything observed so far, and everything that was
  /// synchronized, belongs to the previous account. Starting again resets it once the new account is known.
  @objc func didChangeCloudIdentity() {
    LOG(.info, "The iCloud identity has changed")
    stopSynchronization()
    guard settings.iCLoudSynchronizationEnabled() else { return }
    startSynchronization()
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

extension iCloudSynchronizaionManager: LocalDirectoryMonitorDelegate {
  func didReceiveLocalSnapshot(_ snapshot: LocalSnapshot) {
    processEvents(stateResolver.resolveEvent(.didUpdateLocalContents(snapshot)))
  }

  func didReceiveLocalMonitorError(_ error: Error) {
    processError(error)
  }
}

// MARK: - iCloudStorageManger + CloudDirectoryMonitorDelegate

extension iCloudSynchronizaionManager: CloudDirectoryMonitorDelegate {
  func didReceiveCloudSnapshot(_ snapshot: CloudSnapshot) {
    /* An item error is a property of the cloud directory, not the result of a write: it stands until iCloud
     stops reporting it. Every snapshot carries it, including the ones that repeat the previous one and are not
     reconciled, so this is the only place where the condition is observed exactly. */
    updateSynchronizationError(snapshot.items.compactMap(\.synchronizationError).first)
    processEvents(stateResolver.resolveEvent(.didUpdateCloudContents(snapshot)))
  }

  func didReceiveCloudMonitorError(_ error: Error) {
    processError(error)
  }
}

// MARK: - Private methods

private extension iCloudSynchronizaionManager {
  func processEvents(_ events: [OutgoingSynchronizationEvent]) {
    for event in events {
      guard let fileWriter else {
        // Synchronization was stopped: nothing should wait for a write that will not happen.
        stateResolver.resolveEvent(.didFailWriting(event))
        continue
      }
      fileWriter.processEvent(event, completion: writingResultHandler(for: event))
    }
    if stateResolver.hasPendingConfirmations {
      scheduleConfirmation()
    }
  }

  /// A deletion crosses the queues before it is performed, so what it was decided from is checked once more
  /// against the latest observations: the file that is missing is still missing, by the same confirmed absence,
  /// and no write has been recorded for it since.
  func authorizesDeletion(_ event: OutgoingSynchronizationEvent) -> Bool {
    guard stateResolver.authorizes(event) else {
      LOG(.info, "Skip the outdated deletion: \(event)")
      stateResolver.resolveEvent(.didFailWriting(event))
      return false
    }
    return true
  }

  func writingResultHandler(for event: OutgoingSynchronizationEvent) -> WritingResultCompletionHandler {
    let requestedInSession = session
    return { [weak self] result in
      guard let self else { return }
      if case .reloadCategoriesAtURLs(let urls) = result {
        // The files were replaced on disk: the app has to show what is there, whatever the synchronization does
        // now. Nothing else a result carries is acted on once the session that requested the write has ended.
        urls.forEach { self.bookmarksManager.reloadCategory(atFilePath: $0.path) }
      }
      guard session == requestedInSession else { return }
      switch result {
      case .success, .reloadCategoriesAtURLs:
        break
      case .skipped(let reason):
        // The content that was going to be written must not become the common base: it was not written.
        LOG(.info, "Skipped \(event): \(reason)")
        stateResolver.resolveEvent(.didFailWriting(event))
        return
      case .deleteCategory(let url):
        guard authorizesDeletion(event) else { return }
        // A category that is not loaded, or whose file could not be moved to the trash, is not deleted. Reporting
        // that as a failure keeps the content that was last synchronized, so the file is confirmed and deleted
        // again instead of being uploaded back as a new one.
        guard bookmarksManager.deleteCategory(atFilePath: url.path) else {
          LOG(.warning, "Failed to delete the category: \(event)")
          stateResolver.resolveEvent(.didFailWriting(event))
          return
        }
      case .trashCloudItem(let url, let expectedContent):
        guard authorizesDeletion(event) else { return }
        guard let fileWriter else {
          stateResolver.resolveEvent(.didFailWriting(event))
          return
        }
        // The trashing itself reports the result: this handler is called again with its own outcome.
        fileWriter.trashCloudItem(at: url, expecting: expectedContent, completion: writingResultHandler(for: event))
        return
      case .failure(let error):
        stateResolver.resolveEvent(.didFailWriting(event))
        return processError(error)
      }
      stateResolver.resolveEvent(.didFinishWriting(event))
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
        // A transient condition: it is shown while it lasts and cleared by the next snapshot without it.
        updateSynchronizationError(syncError)
      case .iCloudIsNotAvailable:
        LOG(.warning, "Synchronization Warning: \(error.localizedDescription)")
        stopSynchronization()
      case .failedToOpenLocalDirectoryFileDescriptor,
           .containerNotFound:
        LOG(.error, "Synchronization Error: \(error.localizedDescription)")
        stopSynchronization(withError: error)
      }
    default:
      // An iCloud error that reached the app as a plain NSError means the same thing as the mapped one.
      if let ubiquitousError = error.ubiquitousError {
        return processError(ubiquitousError)
      }
      LOG(.error, "System Error: \(error.localizedDescription)")
      stopSynchronization(withError: error)
    }
  }

  func updateSynchronizationError(_ error: SynchronizationError?) {
    guard synchronizationError as? SynchronizationError != error else { return }
    if let error {
      LOG(.warning, "Synchronization Warning: \(error.localizedDescription)")
    }
    synchronizationError = error
  }
}

// MARK: - Observation

extension iCloudSynchronizaionManager {
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
    for (_, observable) in observers.removeUnreachable() {
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

@objc public extension NSNotification {
  static let iCloudSynchronizationDidChangeEnabledState = Notification.Name.iCloudSynchronizationDidChangeEnabledStateNotification
}

// MARK: - Dictionary + RemoveUnreachable

private extension Dictionary where Key == ObjectIdentifier, Value == iCloudSynchronizaionManager.Observation {
  mutating func removeUnreachable() -> Self {
    for (id, observation) in self {
      if observation.observer == nil {
        removeValue(forKey: id)
      }
    }
    return self
  }
}
