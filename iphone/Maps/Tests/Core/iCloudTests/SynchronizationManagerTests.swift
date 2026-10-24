@testable import Organic_Maps__Debug_
import UIKit
import XCTest

final class SynchronizationManagerTests: XCTestCase {
  private var manager: iCloudSynchronizaionManager!
  private var cloudMonitor: CloudMonitorStub!
  private var localMonitor: LocalMonitorStub!
  private var resolver: ResolverStub!
  private var fingerprintProvider: FingerprintProviderMock!
  private var clock: ActiveSynchronizationClock!
  private var stateStore: SynchronizedStateStoreMock!

  override func setUp() {
    super.setUp()
    cloudMonitor = CloudMonitorStub()
    localMonitor = LocalMonitorStub()
    resolver = ResolverStub()
    fingerprintProvider = FingerprintProviderMock()
    clock = ActiveSynchronizationClock()
    stateStore = SynchronizedStateStoreMock()
    manager = makeManager()
  }

  private func makeManager(fileManager: FileManager = .default,
                           stateResolver: SynchronizationStateResolver? = nil) -> iCloudSynchronizaionManager {
    let manager = iCloudSynchronizaionManager(fileManager: fileManager,
                                              settings: EnabledSynchronizationSettings.self,
                                              bookmarksManager: .shared(),
                                              cloudDirectoryMonitor: cloudMonitor,
                                              localDirectoryMonitor: localMonitor,
                                              stateResolver: stateResolver ?? resolver,
                                              stateStore: stateStore,
                                              fingerprintProvider: fingerprintProvider,
                                              clock: clock)
    manager.start()
    return manager
  }

  override func tearDown() {
    NotificationCenter.default.removeObserver(manager!)
    clock.pause()
    manager = nil
    cloudMonitor = nil
    localMonitor = nil
    resolver = nil
    fingerprintProvider = nil
    clock = nil
    stateStore = nil
    super.tearDown()
  }

  func testStartCompletedInBackgroundResumesItsClockOnForeground() {
    post(UIApplication.didBecomeActiveNotification)
    XCTAssertEqual(cloudMonitor.state, .starting)
    post(UIApplication.didEnterBackgroundNotification)
    XCTAssertFalse(clock.isRunning)

    cloudMonitor.finishStart()
    XCTAssertEqual(localMonitor.state, .paused)
    XCTAssertEqual(cloudMonitor.state, .paused)

    post(UIApplication.didBecomeActiveNotification)

    XCTAssertTrue(clock.isRunning)
    XCTAssertEqual(localMonitor.state, .started)
    XCTAssertEqual(cloudMonitor.state, .started)
    XCTAssertEqual(localMonitor.startCount, 1)
    XCTAssertEqual(cloudMonitor.startCount, 1)
    XCTAssertEqual(localMonitor.refreshCount, 1)
  }

  func testForegroundWhileStartingDoesNotStartTheLocalMonitorEarly() {
    post(UIApplication.didBecomeActiveNotification)
    post(UIApplication.didEnterBackgroundNotification)
    post(UIApplication.didBecomeActiveNotification)

    XCTAssertTrue(clock.isRunning)
    XCTAssertEqual(localMonitor.state, .stopped)
    XCTAssertEqual(cloudMonitor.startCount, 1)

    cloudMonitor.finishStart()

    XCTAssertEqual(localMonitor.startCount, 1)
    XCTAssertEqual(localMonitor.state, .started)
  }

  func testFingerprintCallbackDoesNotReconcilePausedSnapshots() {
    post(UIApplication.didBecomeActiveNotification)
    cloudMonitor.finishStart()
    post(UIApplication.didEnterBackgroundNotification)
    let eventsBeforeCallback = resolver.events.count

    fingerprintProvider.onContentsMayBeKnown?()

    XCTAssertEqual(resolver.events.count, eventsBeforeCallback)

    post(UIApplication.didBecomeActiveNotification)
    guard case .didUpdateKnownContents = resolver.events.last else {
      return XCTFail("Resuming must consume fingerprint results received while paused")
    }
  }

  func testResumeReconcilesKnownContentWithUnchangedSnapshots() throws {
    let previous = try XCTUnwrap(manager)
    NotificationCenter.default.removeObserver(previous)
    let realResolver = iCloudSynchronizationStateResolver(store: stateStore,
                                                          fingerprintProvider: fingerprintProvider, clock: clock)
    manager = makeManager(stateResolver: realResolver)
    let local = LocalMetadataItem.stub(fileName: "paused.kml", size: 1)
    let cloud = CloudMetadataItem.stub(fileName: "paused.kml", size: 1)
    let localSnapshot = LocalSnapshot(items: [local])
    let cloudSnapshot = CloudSnapshot(items: [cloud])
    localMonitor.snapshot = localSnapshot
    cloudMonitor.snapshot = cloudSnapshot
    post(UIApplication.didBecomeActiveNotification)
    cloudMonitor.finishStart()
    manager.didReceiveLocalSnapshot(localSnapshot)
    manager.didReceiveCloudSnapshot(cloudSnapshot)
    XCTAssertNil(stateStore.state(for: "paused.kml"))
    post(UIApplication.didEnterBackgroundNotification)
    fingerprintProvider.contents = [local.fileUrl: "A", cloud.fileUrl: "A"]
    fingerprintProvider.onContentsMayBeKnown?()
    XCTAssertNil(stateStore.state(for: "paused.kml"))

    post(UIApplication.didBecomeActiveNotification)

    XCTAssertEqual(stateStore.state(for: "paused.kml")?.fingerprint, Fingerprint(hashing: Data("A".utf8)))
  }

  func testIncompleteMetadataReportsQuotaAndCannotClearIt() {
    let quota = NSError(domain: NSCocoaErrorDomain, code: NSUbiquitousFileNotUploadedDueToQuotaError)
    let item = MetadataItemMock([
      NSMetadataItemFSNameKey: "quota.kml",
      NSMetadataItemURLKey: URL(fileURLWithPath: "/cloud/quota.kml"),
      NSMetadataItemFSContentChangeDateKey: Date(),
      NSMetadataUbiquitousItemDownloadingStatusKey: NSMetadataUbiquitousItemDownloadingStatusCurrent,
      NSMetadataUbiquitousItemUploadingErrorKey: quota,
    ])
    let snapshot = iCloudDocumentsMonitor.snapshot(of: MetadataQueryMock([item]), in: URL(fileURLWithPath: "/cloud"))
    XCTAssertEqual(snapshot.unavailableFileNames, ["quota.kml"])
    var states = [SynchronizationManagerState]()
    manager.addObserver(self) { states.append($0) }
    manager.didReceiveCloudSnapshot(snapshot)
    manager.didReceiveCloudSnapshot(CloudSnapshot(items: [], isComplete: false))
    manager.didReceiveCloudSnapshot(CloudSnapshot(items: [], unavailableFileNames: ["quota.kml"]))
    let observed = expectation(description: "Quota is retained through incomplete observations")
    DispatchQueue.main.async { observed.fulfill() }
    wait(for: [observed], timeout: 1)
    XCTAssertEqual(states.count, 2)
    XCTAssertEqual(states.last?.error as? SynchronizationError, .fileNotUploadedDueToQuota)

    manager.didReceiveCloudSnapshot(CloudSnapshot(items: []))
    let cleared = expectation(description: "Complete error-free inventory clears quota")
    DispatchQueue.main.async { cleared.fulfill() }
    wait(for: [cleared], timeout: 1)
    XCTAssertNil(states.last?.error)
  }

  func testAccountChangeResetsTheFingerprintProvider() {
    post(UIApplication.didBecomeActiveNotification)
    cloudMonitor.finishStart()

    post(.NSUbiquityIdentityDidChange)

    XCTAssertEqual(fingerprintProvider.resetCount, 1)
    XCTAssertEqual(cloudMonitor.startCount, 2)
  }

  func testCleanSnapshotClearsAnUnmappedSystemError() {
    let error = NSError(domain: NSCocoaErrorDomain, code: NSFileWriteOutOfSpaceError)
    var states = [SynchronizationManagerState]()
    manager.addObserver(self) { states.append($0) }
    manager.updateSynchronizationError(error)
    manager.didReceiveCloudSnapshot(CloudSnapshot(items: []))
    let delivered = expectation(description: "The queued observer callbacks have run")
    DispatchQueue.main.async { delivered.fulfill() }
    wait(for: [delivered], timeout: 1)

    XCTAssertEqual(states.count, 3)
    XCTAssertEqual(states.dropFirst().first?.error, error)
    XCTAssertNil(states.last?.error)
  }

  func testConfirmedDeletionTrashesAFileThatHasNoLoadedCategory() throws {
    let bookmarksManager = BookmarksManager.shared()
    XCTAssertTrue(bookmarksManager.areBookmarksLoaded())
    let name = "UnparsedSyncTests-\(UUID().uuidString).kml"
    let fileUrl = FileManager.default.bookmarksDirectoryUrl.appendingPathComponent(name)
    let trashUrl = fileUrl.deletingLastPathComponent().deletingLastPathComponent()
      .appendingPathComponent(".Trash").appendingPathComponent(name)
    let data = Data("unsupported KML content".utf8)
    try data.write(to: fileUrl)
    addTeardownBlock {
      try? FileManager.default.removeItem(at: fileUrl)
      try? FileManager.default.removeItem(at: trashUrl)
    }

    XCTAssertTrue(bookmarksManager.deleteCategory(atFilePath: fileUrl.path))

    XCTAssertFalse(FileManager.default.fileExists(atPath: fileUrl.path))
    XCTAssertEqual(try Data(contentsOf: trashUrl), data)
    XCTAssertTrue(bookmarksManager.deleteCategory(atFilePath: fileUrl.path), "An absent file is already deleted")
  }

  func testNativeEditDuringReplacementKeepsBothVersionsAndCompletesTheTransfer() throws {
    try checkIncomingTransfer(edit: .duringReplacement)
  }

  func testNativeEditBeforeReloadKeepsBothVersionsAndCompletesTheTransfer() throws {
    try checkIncomingTransfer(edit: .beforeReload)
  }

  func testIncomingTransferReloadsAnUneditedCategoryBeforeCompleting() throws {
    try checkIncomingTransfer(edit: .none)
  }

  func testUnreadableIncomingContentKeepsTheLoadedCategoryAndBothFiles() throws {
    try checkIncomingTransfer(edit: .none, unreadableIncoming: true)
  }

  func testReloadRejectsAnEditMadeAfterItsRequest() throws {
    let bookmarks = BookmarksManager.shared()
    let name = "SyncReload-\(UUID().uuidString)"
    let (categoryId, localUrl) = try categoryFixture(named: name)
    let update = try XCTUnwrap(bookmarks.beginFileUpdate(atPath: localUrl.path))
    let incomingUrl = FileManager.default.temporaryDirectory.appendingPathComponent(name).appendingPathExtension("kml")
    try incomingContents(named: name).write(to: incomingUrl)
    defer { try? FileManager.default.removeItem(at: incomingUrl) }
    let reloaded = expectation(description: "Reload rejects an edit made after the request")
    bookmarks.reloadCategory(with: update, fromFilePath: incomingUrl.path) { applied in
      XCTAssertFalse(applied)
      reloaded.fulfill()
    }
    // The main queue cannot apply the asynchronous read before this edit is made.
    bookmarks.setCategory(categoryId, description: "edit after reload request")
    wait(for: [reloaded], timeout: 5)
    waitForSave(at: localUrl)

    XCTAssertEqual(bookmarks.category(withId: categoryId).detailedAnnotation, "edit after reload request")
    let saved = try Data(contentsOf: localUrl)
    XCTAssertTrue(String(decoding: saved, as: UTF8.self).contains("edit after reload request"))
  }

  private enum LocalEditTiming {
    case none, duringReplacement, beforeReload
  }

  func testPreservedCopyLoadingKeepsEditsToAnAlreadyLoadedCopy() throws {
    let bookmarks = BookmarksManager.shared()
    let name = "Preserved-\(UUID().uuidString)"
    let (categoryId, url) = try categoryFixture(named: name)

    bookmarks.loadCategoryIfNeeded(atFilePath: url.path)
    bookmarks.setCategory(categoryId, description: "edited preserved copy")
    waitForSave(at: url)

    XCTAssertEqual(bookmarks.category(withId: categoryId).detailedAnnotation, "edited preserved copy")
  }

  private func checkIncomingTransfer(edit: LocalEditTiming, unreadableIncoming: Bool = false) throws {
    let bookmarks = BookmarksManager.shared()
    let name = "SyncRace-\(UUID().uuidString)"
    let (categoryId, localUrl) = try categoryFixture(named: name)
    let base = try Fingerprint(hashing: Data(contentsOf: localUrl))
    let cloudDirectory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
    try FileManager.default.createDirectory(at: cloudDirectory, withIntermediateDirectories: true)
    let cloudUrl = cloudDirectory.appendingPathComponent(localUrl.lastPathComponent)
    let incoming = unreadableIncoming ? Data("unparseable incoming content".utf8) : incomingContents(named: name)
    try incoming.write(to: cloudUrl)
    addTeardownBlock { try? FileManager.default.removeItem(at: cloudDirectory) }

    let fileManager = EditingDuringReplacementFileManager()
    if edit == .duringReplacement {
      fileManager.edit = { bookmarks.setCategory(categoryId, description: "new local edit") }
    }
    NotificationCenter.default.removeObserver(manager!)
    localMonitor.directory = FileManager.default.bookmarksDirectoryUrl
    cloudMonitor.directory = cloudDirectory
    manager = makeManager(fileManager: fileManager)
    post(UIApplication.didBecomeActiveNotification)
    cloudMonitor.finishStart()
    if edit == .beforeReload {
      // The writer's accessor queues this edit before its result can request a reload on the main queue.
      fileManager.editAfterReplacement = { bookmarks.setCategory(categoryId, description: "new local edit") }
    }
    let item = CloudMetadataItem(fileName: cloudUrl.lastPathComponent, fileUrl: cloudUrl,
                                 lastModificationDate: 2, size: Int64(incoming.count), isDownloaded: true,
                                 hasUnresolvedConflicts: false, downloadingError: nil, uploadingError: nil)
    let transfer = OutgoingSynchronizationEvent.updateLocalItem(with: item, replacing: base, preservingLocal: false)
    resolver.nextEvents = [transfer]
    let finished = expectation(description: "The raced transfer preserves the incoming version")
    resolver.onEvent = { event in
      guard case .didFinishWriting(let completed, let content, let awaitingObservation) = event else { return }
      XCTAssertEqual(completed, transfer)
      XCTAssertEqual(content, Fingerprint(hashing: incoming))
      XCTAssertEqual(awaitingObservation, edit == .none && !unreadableIncoming)
      finished.fulfill()
    }
    manager.didReceiveCloudSnapshot(CloudSnapshot(items: [item]))
    wait(for: [finished], timeout: 10)
    let loaded = XCTNSPredicateExpectation(predicate: NSPredicate { _, _ in
      bookmarks.areBookmarksLoaded() && !bookmarks.isCategoryFileSaving(atPath: localUrl.path)
    }, object: nil)
    wait(for: [loaded], timeout: 5)

    let expectedDescription = unreadableIncoming ? "original" : (edit == .none ? "remote" : "new local edit")
    XCTAssertEqual(bookmarks.category(withId: categoryId).detailedAnnotation, expectedDescription)
    XCTAssertEqual(bookmarks.sortedUserCategories().filter { $0.title.hasPrefix(name) }.count, edit == .none ? 1 : 2)
    let savedData = try Data(contentsOf: localUrl)
    XCTAssertTrue(String(decoding: savedData, as: UTF8.self).contains(expectedDescription))
    if !unreadableIncoming {
      XCTAssertTrue(bookmarks.sortedUserCategories().contains { $0.title == "\(name) incoming" })
    }
    let copies = try FileManager.default.contentsOfDirectory(at: localUrl.deletingLastPathComponent(), includingPropertiesForKeys: nil)
      .filter { $0.lastPathComponent.hasPrefix("conflict_") }
    if edit != .none || unreadableIncoming {
      let copy = try XCTUnwrap(copies.first { (try? Data(contentsOf: $0)) == incoming })
      if unreadableIncoming {
        addTeardownBlock { try? FileManager.default.removeItem(at: copy) }
      }
    } else {
      XCTAssertEqual(savedData, incoming)
    }
  }

  private func categoryFixture(named name: String) throws -> (MWMMarkGroupID, URL) {
    let bookmarks = BookmarksManager.shared()
    let categoryId = bookmarks.createCategory(withName: name)
    let url = FileManager.default.bookmarksDirectoryUrl.appendingPathComponent(name).appendingPathExtension("kml")
    addTeardownBlock {
      for category in bookmarks.sortedUserCategories() where category.title.hasPrefix(name) {
        bookmarks.deleteCategory(category.categoryId)
      }
      let trashed = bookmarks.getRecentlyDeletedCategories().filter { $0.title.hasPrefix(name) }.map(\.fileURL)
      bookmarks.deleteRecentlyDeletedCategory(at: trashed)
    }
    bookmarks.setCategory(categoryId, description: "original")
    waitForSave(at: url)
    return (categoryId, url)
  }

  private func waitForSave(at url: URL) {
    let saved = XCTNSPredicateExpectation(predicate: NSPredicate { _, _ in
      let bookmarks = BookmarksManager.shared()
      return bookmarks.areBookmarksLoaded() && FileManager.default.fileExists(atPath: url.path)
        && !bookmarks.isCategoryFileSaving(atPath: url.path)
    }, object: nil)
    wait(for: [saved], timeout: 5)
  }

  private func incomingContents(named name: String) -> Data {
    Data("""
    <?xml version="1.0" encoding="UTF-8"?>
    <kml xmlns="http://www.opengis.net/kml/2.2"><Document>
    <name>\(name) incoming</name><description>remote</description>
    <Placemark><name>Remote place</name><Point><coordinates>0,0</coordinates></Point></Placemark>
    </Document></kml>
    """.utf8)
  }

  private func post(_ name: Notification.Name) {
    NotificationCenter.default.post(name: name, object: nil)
  }
}

private final class EditingDuringReplacementFileManager: FileManager {
  var edit: (() -> Void)?
  var editAfterReplacement: (() -> Void)?

  override func replaceItem(at original: URL, withItemAt replacement: URL, backupItemName: String?,
                            options: FileManager.ItemReplacementOptions,
                            resultingItemURL: AutoreleasingUnsafeMutablePointer<NSURL?>?) throws {
    if let edit {
      self.edit = nil
      DispatchQueue.main.sync(execute: edit)
    }
    try super.replaceItem(at: original, withItemAt: replacement, backupItemName: backupItemName,
                          options: options, resultingItemURL: resultingItemURL)
    if let editAfterReplacement {
      self.editAfterReplacement = nil
      DispatchQueue.main.async(execute: editAfterReplacement)
    }
  }
}

private final class EnabledSynchronizationSettings: Settings {
  override class func iCLoudSynchronizationEnabled() -> Bool { true }
}

private final class CloudMonitorStub: CloudDirectoryMonitor {
  weak var delegate: CloudDirectoryMonitorDelegate?
  var state: DirectoryMonitorState = .stopped
  var cloudIdentity: Data? { nil }
  private var startCompletion: ((Result<URL, Error>) -> Void)?
  private(set) var startCount = 0
  var directory = URL(fileURLWithPath: "/cloud")
  var snapshot: CloudSnapshot?

  func start(completion: ((Result<URL, Error>) -> Void)?) {
    state = .starting
    startCount += 1
    startCompletion = completion
  }

  func finishStart() {
    state = .started
    startCompletion?(.success(directory))
    startCompletion = nil
  }

  func stop() { state = .stopped }

  func pause() {
    if state == .started {
      state = .paused
    }
  }

  func resume() {
    if state == .paused {
      state = .started
    }
  }

  func refresh() {
    if state == .started, let snapshot {
      delegate?.didReceiveCloudSnapshot(snapshot)
    }
  }

  func isCloudAvailable() -> Bool { true }
}

private final class LocalMonitorStub: LocalDirectoryMonitor {
  weak var delegate: LocalDirectoryMonitorDelegate?
  var state: DirectoryMonitorState = .stopped
  var directory = URL(fileURLWithPath: "/local")
  var snapshot: LocalSnapshot?
  private(set) var startCount = 0
  private(set) var refreshCount = 0

  func start(completion: ((Result<URL, Error>) -> Void)?) {
    state = .started
    startCount += 1
    completion?(.success(directory))
  }

  func stop() { state = .stopped }

  func pause() {
    if state == .started {
      state = .paused
    }
  }

  func resume() {
    if state == .paused {
      state = .started
    }
  }

  func refresh() {
    if state == .started {
      refreshCount += 1
      if let snapshot {
        delegate?.didReceiveLocalSnapshot(snapshot)
      }
    }
  }
}

private final class ResolverStub: SynchronizationStateResolver {
  let hasPendingConfirmations = false
  private(set) var events = [IncomingSynchronizationEvent]()
  var nextEvents = [OutgoingSynchronizationEvent]()
  var onEvent: ((IncomingSynchronizationEvent) -> Void)?

  func resolveEvent(_ event: IncomingSynchronizationEvent) -> [OutgoingSynchronizationEvent] {
    events.append(event)
    onEvent?(event)
    defer { nextEvents = [] }
    return nextEvents
  }

  func authorizes(_: OutgoingSynchronizationEvent) -> Bool { true }
  func resetState() {}
}
