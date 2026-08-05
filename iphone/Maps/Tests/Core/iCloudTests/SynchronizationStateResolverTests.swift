@testable import Organic_Maps__Debug_
import XCTest

final class SynchronizationStateResolverTests: XCTestCase {
  private var clock: SynchronizationClockMock!
  private var fingerprintProvider: FingerprintProviderMock!
  private var store: SynchronizedStateStoreMock!
  private var resolver: iCloudSynchronizationStateResolver!

  override func setUp() {
    super.setUp()
    clock = SynchronizationClockMock()
    fingerprintProvider = FingerprintProviderMock()
    store = SynchronizedStateStoreMock()
    resolver = iCloudSynchronizationStateResolver(store: store,
                                                  fingerprintProvider: fingerprintProvider,
                                                  clock: clock)
  }

  override func tearDown() {
    resolver = nil
    store = nil
    fingerprintProvider = nil
    clock = nil
    super.tearDown()
  }

  // MARK: - Both directories are observed before anything is written

  func testNoEventsUntilBothDirectoriesAreObserved() {
    XCTAssertTrue(update(local: [local("file.kml", "A")]).isEmpty)
    XCTAssertEqual(update(cloud: []), [.createCloudItem(with: local("file.kml", "A"))])
  }

  func testEmptyDirectoriesProduceNoEvents() {
    XCTAssertTrue(update(local: []).isEmpty)
    XCTAssertTrue(update(cloud: []).isEmpty)
  }

  func testFilesAreReconciledIndependently() {
    let unchangedLocal = local("unchanged.kml", "A")
    let localOnly = local("local.kml", "B")
    let cloudOnly = cloud("cloud.kml", "C")

    XCTAssertTrue(update(local: [unchangedLocal, localOnly]).isEmpty)
    let events = update(cloud: [cloud("unchanged.kml", "A"), cloudOnly])
    XCTAssertEqual(events.count, 2)
    XCTAssertTrue(events.contains(.createCloudItem(with: localOnly)))
    XCTAssertTrue(events.contains(.createLocalItem(with: cloudOnly)))
    XCTAssertEqual(store.state(for: "unchanged.kml")?.fingerprint, fingerprint("A"))
  }

  // MARK: - Content decides which side wins

  func testEqualContentWithDifferentDatesIsSynchronizedWithoutConflict() {
    let localItem = local("file.kml", "A", modified: 1)
    let cloudItem = cloud("file.kml", "A", modified: 42)

    XCTAssertTrue(update(local: [localItem]).isEmpty)
    XCTAssertTrue(update(cloud: [cloudItem]).isEmpty)
    XCTAssertEqual(store.state(for: "file.kml")?.fingerprint, fingerprint("A"))
  }

  func testChangedLocalFileIsUploaded() {
    synchronize("file.kml", content: "A")

    let changedLocalItem = local("file.kml", "B", modified: 2)
    XCTAssertEqual(update(local: [changedLocalItem]), [.updateCloudItem(with: changedLocalItem)])
  }

  func testChangedCloudFileIsDownloaded() {
    synchronize("file.kml", content: "A")

    let changedCloudItem = cloud("file.kml", "B", modified: 2)
    XCTAssertEqual(update(cloud: [changedCloudItem]), [.updateLocalItem(with: changedCloudItem, preserving: nil)])
  }

  func testFileChangedOnBothSidesKeepsBothVersions() {
    synchronize("file.kml", content: "A")

    let changedLocalItem = local("file.kml", "B", modified: 2)
    XCTAssertEqual(update(local: [changedLocalItem]), [.updateCloudItem(with: changedLocalItem)])

    // The file was changed on another device before the local change was uploaded. The local version is preserved
    // by the same operation that overwrites it, so a failure to keep it cannot be followed by its destruction.
    let changedCloudItem = cloud("file.kml", "C", modified: 3)
    XCTAssertEqual(update(cloud: [changedCloudItem]),
                   [.updateLocalItem(with: changedCloudItem, preserving: changedLocalItem)])
  }

  func testDownloadIsRequestedAgainWhenICloudDoesNotAct() {
    let cloudItem = cloud("file.kml", "A", isDownloaded: false)
    XCTAssertTrue(update(local: []).isEmpty)

    XCTAssertEqual(update(cloud: [cloudItem]), [.startDownloading(cloudItem)])
    XCTAssertTrue(resolver.hasPendingConfirmations, "The file waits for something a new snapshot has to report")
    XCTAssertTrue(update(cloud: [cloudItem]).isEmpty, "iCloud reports the progress a lot: do not ask on every one")
    clock.advance(by: 1)
    XCTAssertTrue(update(cloud: [cloudItem]).isEmpty)

    // Nothing happened for a while -- a paused account, Low Data Mode, no space -- so the request is repeated
    // instead of leaving the file unsynchronized for the rest of the session.
    clock.advance(by: iCloudSynchronizationStateResolver.Constants.requestRepeatInterval)
    XCTAssertEqual(update(cloud: [cloudItem]), [.startDownloading(cloudItem)])

    let downloadedItem = cloud("file.kml", "A")
    XCTAssertEqual(update(cloud: [downloadedItem]), [.createLocalItem(with: downloadedItem)])
    clock.advance(by: iCloudSynchronizationStateResolver.Constants.requestRepeatInterval)
    XCTAssertTrue(update(cloud: [downloadedItem]).isEmpty, "The file is downloaded: nothing is requested again")
  }

  // MARK: - A file is deleted only when its absence is confirmed

  func testRemoteDeletionRequiresTwoStableObservations() {
    synchronize("file.kml", content: "A")

    let absentSince = startAbsenceConfirmation { update(cloud: []) }
    XCTAssertEqual(update(cloud: []),
                   [.removeLocalItem(local("file.kml", "A"), evidence(absentSince: absentSince, content: "A"))])
  }

  func testRemovedFileThatReappearsIsNotDeleted() {
    synchronize("file.kml", content: "A")

    startAbsenceConfirmation { update(cloud: []) }
    XCTAssertTrue(update(cloud: [cloud("file.kml", "A")]).isEmpty, "The file is back: nothing to delete")

    // The confirmation starts from scratch.
    startAbsenceConfirmation { update(cloud: []) }
    XCTAssertEqual(update(cloud: []).count, 1)
  }

  func testRemovalDuringOwnUploadIsIgnored() {
    let localItem = local("file.kml", "A")
    XCTAssertTrue(update(local: [localItem]).isEmpty)
    XCTAssertEqual(update(cloud: []), [.createCloudItem(with: localItem)])
    resolver.resolveEvent(.didFinishWriting(.createCloudItem(with: localItem)))

    // iCloud removes the file while it is being replaced and reports it as missing for a while.
    for _ in 0 ..< 5 {
      XCTAssertTrue(update(cloud: []).isEmpty, "The file the app has just written must not be deleted")
      clock.advance(by: iCloudSynchronizationStateResolver.Constants.absenceConfirmationInterval)
    }

    // The upload settles and the file is synchronized.
    XCTAssertTrue(update(cloud: [cloud("file.kml", "A")]).isEmpty)
    XCTAssertEqual(store.state(for: "file.kml")?.fingerprint, fingerprint("A"))
  }

  func testLocalChangeDuringOwnUploadIsUploadedOnce() {
    let localItem = local("file.kml", "A")
    XCTAssertTrue(update(local: [localItem]).isEmpty)
    XCTAssertEqual(update(cloud: []), [.createCloudItem(with: localItem)])
    resolver.resolveEvent(.didFinishWriting(.createCloudItem(with: localItem)))

    let changedLocalItem = local("file.kml", "B", modified: 2)
    XCTAssertEqual(update(local: [changedLocalItem]), [.createCloudItem(with: changedLocalItem)])
    XCTAssertTrue(update(local: [changedLocalItem]).isEmpty, "The same content must not be uploaded twice")
  }

  func testIncompleteSnapshotCancelsPendingDeletion() {
    synchronize("file.kml", content: "A")

    startAbsenceConfirmation { update(cloud: []) }
    XCTAssertTrue(update(cloud: [], isComplete: false).isEmpty, "An incomplete snapshot proves nothing")
    XCTAssertTrue(update(cloud: []).isEmpty, "The confirmation starts from scratch")
  }

  func testUnavailableCloudItemCancelsPendingDeletion() {
    synchronize("file.kml", content: "A")

    startAbsenceConfirmation { update(cloud: []) }
    XCTAssertTrue(update(cloud: [], unavailable: ["file.kml"]).isEmpty, "The file is there, it is just not usable")
    XCTAssertTrue(update(cloud: []).isEmpty)
  }

  func testDeletionIsNotAuthorizedAfterTheFileIsObservedAgain() throws {
    synchronize("file.kml", content: "A")

    startAbsenceConfirmation { update(cloud: []) }
    let deletion = try XCTUnwrap(update(cloud: []).first)
    XCTAssertTrue(resolver.authorizes(deletion))

    XCTAssertTrue(update(cloud: [cloud("file.kml", "A")]).isEmpty)
    XCTAssertFalse(resolver.authorizes(deletion), "The file is back: the deletion is outdated")
  }

  func testDeletionIsNotAuthorizedByAnAbsenceThatWasNeverConfirmed() throws {
    synchronize("file.kml", content: "A")

    startAbsenceConfirmation { update(cloud: []) }
    let deletion = try XCTUnwrap(update(cloud: []).first)

    // The file is observed again and disappears once more while the deletion is crossing the queues. The new
    // absence lasts long enough to look confirmed, but nobody ever confirmed it with a second observation.
    XCTAssertTrue(update(cloud: [cloud("file.kml", "A")]).isEmpty)
    XCTAssertTrue(update(cloud: []).isEmpty, "The confirmation starts from scratch")
    clock.advance(by: iCloudSynchronizationStateResolver.Constants.absenceConfirmationInterval)
    XCTAssertFalse(resolver.authorizes(deletion), "The confirmed absence is not the one standing now")
  }

  func testDeletionIsNotAuthorizedWhenTheSynchronizedContentChanged() throws {
    synchronize("file.kml", content: "A")

    startAbsenceConfirmation { update(cloud: []) }
    let deletion = try XCTUnwrap(update(cloud: []).first)
    XCTAssertTrue(resolver.authorizes(deletion))

    // A write completed while the deletion was crossing the queues and made another content the common base.
    store.setState(SynchronizedFileState(fingerprint: fingerprint("B")), for: "file.kml")
    XCTAssertFalse(resolver.authorizes(deletion), "The surviving copy no longer holds the synchronized content")
  }

  func testCloudDeletionIsAuthorizedByTheSameRule() throws {
    synchronize("file.kml", content: "A")

    startAbsenceConfirmation { update(local: []) }
    let deletion = try XCTUnwrap(update(local: []).first)
    XCTAssertTrue(resolver.authorizes(deletion))

    XCTAssertTrue(update(local: [local("file.kml", "A")]).isEmpty)
    XCTAssertFalse(resolver.authorizes(deletion), "The file is back: trashing the cloud copy is outdated")
  }

  func testCommonBaseIsKeptUntilBothSidesReportTheFileGone() throws {
    synchronize("file.kml", content: "A")

    startAbsenceConfirmation { update(cloud: []) }
    let deletion = try XCTUnwrap(update(cloud: []).first)

    // The deletion reported success, but the category was not loaded and the file is still in the directory.
    resolver.resolveEvent(.didFinishWriting(deletion))
    XCTAssertEqual(store.state(for: "file.kml")?.fingerprint, fingerprint("A"))
    XCTAssertTrue(update(local: [local("file.kml", "A")]).isEmpty, "The file must not be uploaded back as a new one")

    // The common base is forgotten only when nobody holds the file anymore.
    XCTAssertTrue(update(local: []).isEmpty)
    XCTAssertNil(store.state(for: "file.kml"))
  }

  func testRemoteDeletionOfALocallyChangedFilePreservesIt() {
    synchronize("file.kml", content: "A")

    // The file is deleted on another device and changed here before the deletion is confirmed.
    startAbsenceConfirmation { update(cloud: []) }
    let changedLocalItem = local("file.kml", "B", modified: 2)
    XCTAssertEqual(update(local: [changedLocalItem]), [.createCloudItem(with: changedLocalItem)])

    for _ in 0 ..< 5 {
      XCTAssertTrue(update(cloud: []).isEmpty, "The changed local file must not be deleted")
      clock.advance(by: iCloudSynchronizationStateResolver.Constants.absenceConfirmationInterval)
    }
  }

  func testLocalDeletionRemovesTheCloudFileAfterConfirmation() {
    synchronize("file.kml", content: "A")

    let absentSince = startAbsenceConfirmation { update(local: []) }
    XCTAssertEqual(update(local: []),
                   [.removeCloudItem(cloud("file.kml", "A"), evidence(absentSince: absentSince, content: "A"))])
  }

  func testFileWrittenLocallyIsNotUploadedBack() {
    let cloudItem = cloud("file.kml", "A")
    XCTAssertTrue(update(local: []).isEmpty)
    XCTAssertEqual(update(cloud: [cloudItem]), [.createLocalItem(with: cloudItem)])
    resolver.resolveEvent(.didFinishWriting(.createLocalItem(with: cloudItem)))

    // The local monitor reports the file the app has just written itself.
    XCTAssertTrue(update(local: [local("file.kml", "A", modified: 7)]).isEmpty)
    XCTAssertEqual(store.state(for: "file.kml")?.fingerprint, fingerprint("A"))
  }

  func testFailedWriteIsRetried() {
    let localItem = local("file.kml", "A")
    XCTAssertTrue(update(local: [localItem]).isEmpty)
    XCTAssertEqual(update(cloud: []), [.createCloudItem(with: localItem)])

    resolver.resolveEvent(.didFailWriting(.createCloudItem(with: localItem)))
    XCTAssertEqual(update(cloud: []), [.createCloudItem(with: localItem)])
  }

  func testFileRemovedOnBothSidesIsForgotten() {
    synchronize("file.kml", content: "A")

    XCTAssertTrue(update(local: []).isEmpty)
    XCTAssertTrue(update(cloud: []).isEmpty)
    XCTAssertNil(store.state(for: "file.kml"))
    XCTAssertFalse(resolver.hasPendingConfirmations)
  }

  // MARK: - Metadata observations

  func testItemWithoutUrlIsPresentButNotActionable() {
    let observation = CloudMetadataItem.observation(from: MetadataItemMock([NSMetadataItemFSNameKey: "file.kml"]))
    guard case .unusable(let fileName, let fileUrl, let missingAttributes) = observation else {
      return XCTFail("A named item without other attributes must be observed as unusable")
    }
    XCTAssertEqual(fileName, "file.kml")
    XCTAssertNil(fileUrl)
    XCTAssertTrue(missingAttributes.contains(NSMetadataItemURLKey))
  }

  func testItemWithoutNameIsIdentifiedByUrl() {
    let observation = CloudMetadataItem.observation(from: MetadataItemMock([
      NSMetadataItemURLKey: URL(fileURLWithPath: "/cloud/file.kml"),
    ]))
    guard case .unusable(let fileName, _, _) = observation else {
      return XCTFail("An item with a URL must be observed as unusable and not as unidentifiable")
    }
    XCTAssertEqual(fileName, "file.kml")
  }

  func testUnusableItemOutsideOfTheDirectoryDoesNotStandForTheFileInIt() {
    let directory = URL(fileURLWithPath: "/cloud")
    // A copy iCloud is moving to the trash: its attributes are already gone, but its URL is not the directory.
    let query = MetadataQueryMock([MetadataItemMock([
      NSMetadataItemFSNameKey: "file.kml",
      NSMetadataItemURLKey: URL(fileURLWithPath: "/cloud/.Trash/file.kml"),
    ])])

    let snapshot = iCloudDocumentsMonitor.snapshot(of: query, in: directory)
    XCTAssertTrue(snapshot.unavailableFileNames.isEmpty)
    XCTAssertTrue(snapshot.isComplete)
    guard case .absent = snapshot.state(of: "file.kml") else {
      return XCTFail("A file that is only in the trash is absent from the directory")
    }
  }

  func testUnusableItemInTheDirectoryProvesThatTheFileIsThere() {
    let directory = URL(fileURLWithPath: "/cloud")
    let query = MetadataQueryMock([MetadataItemMock([
      NSMetadataItemFSNameKey: "file.kml",
      NSMetadataItemURLKey: URL(fileURLWithPath: "/cloud/file.kml"),
    ])])

    let snapshot = iCloudDocumentsMonitor.snapshot(of: query, in: directory)
    XCTAssertEqual(snapshot.unavailableFileNames, ["file.kml"])
    guard case .unavailable = snapshot.state(of: "file.kml") else {
      return XCTFail("The file exists but cannot be used yet")
    }
  }

  func testItemWithoutNameAndUrlIsUnidentifiable() {
    let observation = CloudMetadataItem.observation(from: MetadataItemMock([:]))
    guard case .unidentifiable = observation else {
      return XCTFail("An item that cannot be identified must not be attributed to any file")
    }
  }

  /// iCloud reports a file and its trashed copy under the same name until the trashed one is indexed away.
  func testTheMostRecentFileWinsWhenTheSameNameIsObservedTwice() {
    let older = CloudMetadataItem.stub(fileName: "file.kml", lastModificationDate: 1)
    let newer = CloudMetadataItem.stub(fileName: "file.kml", lastModificationDate: 2)

    let snapshot = CloudSnapshot(items: [newer, older])
    XCTAssertEqual(snapshot.items, [newer])
    guard case .present(let item) = snapshot.state(of: "file.kml") else {
      return XCTFail("The file must be present in the snapshot")
    }
    XCTAssertEqual(item, newer)
  }

  func testCompleteItemIsActionable() {
    let observation = CloudMetadataItem.observation(from: MetadataItemMock([
      NSMetadataItemFSNameKey: "file.kml",
      NSMetadataItemURLKey: URL(fileURLWithPath: "/cloud/file.kml"),
      NSMetadataItemFSContentChangeDateKey: Date(timeIntervalSince1970: 100.7),
      NSMetadataItemFSSizeKey: NSNumber(value: 42),
      NSMetadataUbiquitousItemDownloadingStatusKey: NSMetadataUbiquitousItemDownloadingStatusCurrent,
    ]))
    guard case .actionable(let item) = observation else {
      return XCTFail("An item with all the required attributes must be actionable")
    }
    XCTAssertEqual(item.fileName, "file.kml")
    // Not truncated to a whole second: two changes within the same second would look like one.
    XCTAssertEqual(item.lastModificationDate, 100.7, accuracy: 0.000001)
    XCTAssertEqual(item.size, 42)
    XCTAssertTrue(item.isDownloaded)
    XCTAssertFalse(item.hasUnresolvedConflicts)
  }

  // MARK: - Helpers

  private func local(_ fileName: String, _ content: String, modified: TimeInterval = 1) -> LocalMetadataItem {
    let item = LocalMetadataItem.stub(fileName: fileName, lastModificationDate: modified, size: Int64(content.count))
    fingerprintProvider.contents[item.fileUrl] = content
    return item
  }

  private func cloud(_ fileName: String,
                     _ content: String,
                     modified: TimeInterval = 1,
                     isDownloaded: Bool = true) -> CloudMetadataItem {
    let item = CloudMetadataItem.stub(fileName: fileName,
                                      lastModificationDate: modified,
                                      size: Int64(content.count),
                                      isDownloaded: isDownloaded)
    // The content of a file that is not downloaded yet cannot be read.
    fingerprintProvider.contents[item.fileUrl] = isDownloaded ? content : nil
    return item
  }

  private func fingerprint(_ content: String) -> Fingerprint { Fingerprint(hashing: Data(content.utf8)) }

  /// The first snapshot without the file only starts the confirmation: a deletion needs the interval to pass.
  /// @returns When the absence started, as the resolver reports it in the evidence of a deletion.
  @discardableResult
  private func startAbsenceConfirmation(_ observe: () -> [OutgoingSynchronizationEvent],
                                        file: StaticString = #filePath, line: UInt = #line) -> TimeInterval {
    let absentSince = clock.activeTime
    XCTAssertTrue(observe().isEmpty, "The first absence must not delete anything", file: file, line: line)
    clock.advance(by: iCloudSynchronizationStateResolver.Constants.absenceConfirmationInterval)
    return absentSince
  }

  private func evidence(absentSince: TimeInterval, content: String) -> DeletionEvidence {
    DeletionEvidence(absentSince: absentSince, base: fingerprint(content))
  }

  private func update(local items: LocalContents) -> [OutgoingSynchronizationEvent] {
    resolver.resolveEvent(.didUpdateLocalContents(LocalSnapshot(items: items)))
  }

  private func update(cloud items: CloudContents,
                      unavailable: Set<String> = [],
                      isComplete: Bool = true) -> [OutgoingSynchronizationEvent] {
    resolver.resolveEvent(.didUpdateCloudContents(CloudSnapshot(items: items,
                                                                unavailableFileNames: unavailable,
                                                                isComplete: isComplete)))
  }

  /// Brings a file to the synchronized state: both directories have the same content.
  private func synchronize(_ fileName: String, content: String) {
    XCTAssertTrue(update(local: [local(fileName, content)]).isEmpty)
    XCTAssertTrue(update(cloud: [cloud(fileName, content)]).isEmpty)
    XCTAssertEqual(store.state(for: fileName)?.fingerprint, fingerprint(content))
  }
}
