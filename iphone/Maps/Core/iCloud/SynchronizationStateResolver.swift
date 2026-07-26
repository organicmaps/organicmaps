typealias LocalContents = [LocalMetadataItem]
typealias CloudContents = [CloudMetadataItem]
typealias LocalSnapshot = DirectorySnapshot<LocalMetadataItem>
typealias CloudSnapshot = DirectorySnapshot<CloudMetadataItem>

/// How long a file must stay missing, in active synchronization time, before it is deleted on the other side.
/// The directories are observed anew after this interval: a file that stays missing is reported by nobody.
let kAbsenceConfirmationInterval: TimeInterval = 30

protocol SynchronizationStateResolver {
  /// True while some file waits for a confirmation that can only come from a fresh snapshot.
  var hasPendingConfirmations: Bool { get }

  /// Turns an observation into the file operations it requires. Feedback about a finished operation is an
  /// observation too and never produces new events on its own.
  @discardableResult
  func resolveEvent(_ event: IncomingSynchronizationEvent) -> [OutgoingSynchronizationEvent]
  /// Tells whether an event that was resolved earlier is still valid. A destructive event crosses queues before
  /// it is executed, so its preconditions must be checked again against the latest observations.
  func authorizes(_ event: OutgoingSynchronizationEvent) -> Bool
  func resetState()
}

enum IncomingSynchronizationEvent {
  case didUpdateLocalContents(LocalSnapshot)
  case didUpdateCloudContents(CloudSnapshot)
  case didFinishWriting(OutgoingSynchronizationEvent)
  case didFailWriting(OutgoingSynchronizationEvent)
}

enum OutgoingSynchronizationEvent: Equatable {
  case startDownloading(CloudMetadataItem)

  case createLocalItem(with: CloudMetadataItem)
  /// Replaces the local file with the cloud one. When the local version holds changes that were never
  /// synchronized, it is the only copy of them and is preserved under a new name by the same operation.
  case updateLocalItem(with: CloudMetadataItem, preserving: LocalMetadataItem?)
  case removeLocalItem(LocalMetadataItem)

  case createCloudItem(with: LocalMetadataItem)
  case updateCloudItem(with: LocalMetadataItem)
  case removeCloudItem(CloudMetadataItem)

  case resolveVersionsConflict(CloudMetadataItem)
  case didReceiveError(SynchronizationError)

  var fileName: String? {
    switch self {
    case .startDownloading(let item), .createLocalItem(let item), .updateLocalItem(let item, _),
         .removeCloudItem(let item), .resolveVersionsConflict(let item):
      return item.fileName
    case .createCloudItem(let item), .updateCloudItem(let item), .removeLocalItem(let item):
      return item.fileName
    case .didReceiveError:
      return nil
    }
  }
}

/// Reconciles the local and the cloud directory file by file.
///
/// Every incoming snapshot is an observation, not a command: the resolver keeps the state of each file and
/// derives what has to be written from the content of both sides, comparing them with the content that was
/// last synchronized. Deletions are the only irreversible operation and require a confirmed absence.
final class iCloudSynchronizationStateResolver: SynchronizationStateResolver {
  private enum Constants {
    /// A write that iCloud never confirms should not block deletions forever.
    static let writeSettlingInterval: TimeInterval = 300
    /// iCloud reports the downloading progress with a lot of notifications: do not repeat requests on each one.
    static let requestRepeatInterval: TimeInterval = 60
  }

  /// A file operation started by the app. Until iCloud reports its result, the churn it causes on the other side
  /// (a file that briefly disappears while it is being replaced) must not be mistaken for a user action.
  private struct OwnedWrite {
    let fingerprint: Fingerprint
    let startedAt: TimeInterval
  }

  private struct FileState {
    var ownedLocalWrite: OwnedWrite?
    var ownedCloudWrite: OwnedWrite?
    /// When the file was first seen missing, in active synchronization time.
    var localAbsentSince: TimeInterval?
    var cloudAbsentSince: TimeInterval?
    var downloadRequestedAt: TimeInterval?
    var conflictResolutionRequestedAt: TimeInterval?

    /// True while the file waits for something that only a new snapshot can confirm.
    var isPending: Bool {
      localAbsentSince != nil || cloudAbsentSince != nil || ownedLocalWrite != nil || ownedCloudWrite != nil
    }
  }

  private let store: SynchronizedStateStore
  private let fingerprintProvider: FingerprintProvider
  private let clock: SynchronizationClock
  private var states = [String: FileState]()
  private var localSnapshot: LocalSnapshot?
  private var cloudSnapshot: CloudSnapshot?
  /// Set by everything that changes the state without being a snapshot: the next snapshot has to be reconciled
  /// even when it repeats the previous one.
  private var mustReconcile = true

  init(store: SynchronizedStateStore,
       fingerprintProvider: FingerprintProvider = FileContentFingerprintProvider(),
       clock: SynchronizationClock) {
    self.store = store
    self.fingerprintProvider = fingerprintProvider
    self.clock = clock
  }

  // MARK: - SynchronizationStateResolver

  var hasPendingConfirmations: Bool { states.values.contains(where: \.isPending) }

  @discardableResult
  func resolveEvent(_ event: IncomingSynchronizationEvent) -> [OutgoingSynchronizationEvent] {
    switch event {
    case .didUpdateLocalContents(let snapshot):
      // iCloud repeats a snapshot on every uploading and downloading step: reconcile it only when it brings
      // something new or when a file still waits for something.
      guard snapshot != localSnapshot || mustReconcile || hasPendingConfirmations else { return [] }
      localSnapshot = snapshot
    case .didUpdateCloudContents(let snapshot):
      guard snapshot != cloudSnapshot || mustReconcile || hasPendingConfirmations else { return [] }
      cloudSnapshot = snapshot
    case .didFinishWriting(let event):
      finishWriting(event, isSuccessful: true)
      return []
    case .didFailWriting(let event):
      finishWriting(event, isSuccessful: false)
      return []
    }

    let outgoingEvents = reconcile()
    if !outgoingEvents.isEmpty {
      LOG(.info, "Events to process (\(outgoingEvents.count)):")
      outgoingEvents.forEach { LOG(.info, $0) }
    }
    return outgoingEvents
  }

  func authorizes(_ event: OutgoingSynchronizationEvent) -> Bool {
    switch event {
    case .removeLocalItem(let item):
      return states[item.fileName]?.cloudAbsentSince != nil
    case .removeCloudItem(let item):
      return states[item.fileName]?.localAbsentSince != nil
    default:
      return true
    }
  }

  func resetState() {
    LOG(.debug, "Resetting state")
    states.removeAll()
    localSnapshot = nil
    cloudSnapshot = nil
    mustReconcile = true
  }

  // MARK: - Reconciliation

  private func reconcile() -> [OutgoingSynchronizationEvent] {
    // Both directories must be observed at least once: an unobserved directory looks empty.
    guard let localSnapshot, let cloudSnapshot else { return [] }
    mustReconcile = false

    var events = cloudSnapshot.items.compactMap(\.synchronizationError).map { OutgoingSynchronizationEvent.didReceiveError($0) }
    for fileName in localSnapshot.fileNames.union(cloudSnapshot.fileNames).union(states.keys) {
      let local = localSnapshot.state(of: fileName)
      let cloud = cloudSnapshot.state(of: fileName)
      var state = states[fileName] ?? FileState()
      expire(&state.ownedLocalWrite)
      expire(&state.ownedCloudWrite)
      events.append(contentsOf: resolve(fileName, local: local, cloud: cloud, state: &state))
      if case .absent = local, case .absent = cloud {
        states.removeValue(forKey: fileName)
      } else {
        states[fileName] = state
      }
    }
    return events
  }

  private func resolve(_ fileName: String,
                       local: LocalSnapshot.ItemState,
                       cloud: CloudSnapshot.ItemState,
                       state: inout FileState) -> [OutgoingSynchronizationEvent] {
    if case .present(let cloudItem) = cloud, cloudItem.hasUnresolvedConflicts {
      cancelAbsences(&state)
      return shouldRequest(&state.conflictResolutionRequestedAt) ? [.resolveVersionsConflict(cloudItem)] : []
    }

    switch (local, cloud) {
    case (.present(let localItem), .present(let cloudItem)):
      cancelAbsences(&state)
      return resolvePresentOnBothSides(localItem, cloudItem, &state)
    case (.present(let localItem), .absent):
      state.localAbsentSince = nil
      return resolveMissingCloudFile(localItem, &state)
    case (.absent, .present(let cloudItem)):
      state.cloudAbsentSince = nil
      return resolveMissingLocalFile(cloudItem, &state)
    case (.absent, .absent):
      store.setState(nil, for: fileName)
      return []
    case (.unavailable, _), (.unknown, _), (_, .unavailable), (_, .unknown):
      // At least one side is present but unusable, or was not observed in full. Nothing can be concluded and a
      // deletion that is being confirmed is cancelled: the file may well be there.
      cancelAbsences(&state)
      return []
    }
  }

  private func resolvePresentOnBothSides(_ localItem: LocalMetadataItem,
                                         _ cloudItem: CloudMetadataItem,
                                         _ state: inout FileState) -> [OutgoingSynchronizationEvent] {
    guard cloudItem.isDownloaded else { return requestDownload(cloudItem, &state) }
    state.downloadRequestedAt = nil

    let fileName = localItem.fileName
    let synchronized = store.state(for: fileName)
    // Files that still look the way they did when they were synchronized do not have to be read again.
    if let synchronized, synchronized.localIdentity == localItem.identity,
       synchronized.cloudIdentity == cloudItem.identity {
      settle(&state.ownedLocalWrite, observed: synchronized.fingerprint)
      settle(&state.ownedCloudWrite, observed: synchronized.fingerprint)
      return []
    }
    // A file that cannot be read now (it is being replaced by iCloud) is reconciled on the next snapshot.
    guard let localFingerprint = fingerprintProvider.fingerprint(of: localItem),
          let cloudFingerprint = fingerprintProvider.fingerprint(of: cloudItem)
    else { return [] }

    settle(&state.ownedLocalWrite, observed: localFingerprint)
    settle(&state.ownedCloudWrite, observed: cloudFingerprint)

    guard localFingerprint != cloudFingerprint else {
      store.setState(SynchronizedFileState(fingerprint: localFingerprint,
                                           localIdentity: localItem.identity,
                                           cloudIdentity: cloudItem.identity),
                     for: fileName)
      return []
    }
    if synchronized?.fingerprint == localFingerprint {
      return write(.updateLocalItem(with: cloudItem, preserving: nil), content: cloudFingerprint, to: &state.ownedLocalWrite)
    }
    if synchronized?.fingerprint == cloudFingerprint {
      return write(.updateCloudItem(with: localItem), content: localFingerprint, to: &state.ownedCloudWrite)
    }
    // Both sides changed since they were synchronized, or they never were: the local version is the only copy of
    // its changes, so the same operation preserves it under a new name before overwriting it.
    let events = write(.updateLocalItem(with: cloudItem, preserving: localItem),
                       content: cloudFingerprint,
                       to: &state.ownedLocalWrite)
    if !events.isEmpty {
      LOG(.warning, """
      \(fileName) was changed on both sides (local \(localFingerprint), cloud \(cloudFingerprint), \
      synchronized \(synchronized?.fingerprint.description ?? "never")). The local version is kept under a new name.
      """)
    }
    return events
  }

  private func resolveMissingCloudFile(_ localItem: LocalMetadataItem,
                                       _ state: inout FileState) -> [OutgoingSynchronizationEvent] {
    guard let localFingerprint = fingerprintProvider.fingerprint(of: localItem) else { return [] }
    settle(&state.ownedLocalWrite, observed: localFingerprint)

    let fileName = localItem.fileName
    guard store.state(for: fileName)?.fingerprint == localFingerprint else {
      // The file was never synchronized or was changed after that: it is a new version to upload, not a deletion.
      state.cloudAbsentSince = nil
      return write(.createCloudItem(with: localItem), content: localFingerprint, to: &state.ownedCloudWrite)
    }
    guard state.ownedCloudWrite == nil else {
      // iCloud removes the file while it replaces it. Our own write is not a deletion.
      state.cloudAbsentSince = nil
      return []
    }
    guard confirmAbsence(&state.cloudAbsentSince) else { return [] }
    return [.removeLocalItem(localItem)]
  }

  private func resolveMissingLocalFile(_ cloudItem: CloudMetadataItem,
                                       _ state: inout FileState) -> [OutgoingSynchronizationEvent] {
    guard cloudItem.isDownloaded else { return requestDownload(cloudItem, &state) }
    state.downloadRequestedAt = nil
    guard let cloudFingerprint = fingerprintProvider.fingerprint(of: cloudItem) else { return [] }
    settle(&state.ownedCloudWrite, observed: cloudFingerprint)

    let fileName = cloudItem.fileName
    guard store.state(for: fileName)?.fingerprint == cloudFingerprint else {
      // The file is new or was changed in iCloud after the local one was deleted: it is restored, not deleted.
      state.localAbsentSince = nil
      return write(.createLocalItem(with: cloudItem), content: cloudFingerprint, to: &state.ownedLocalWrite)
    }
    guard state.ownedLocalWrite == nil else {
      state.localAbsentSince = nil
      return []
    }
    guard confirmAbsence(&state.localAbsentSince) else { return [] }
    return [.removeCloudItem(cloudItem)]
  }

  // MARK: - File state

  private func cancelAbsences(_ state: inout FileState) {
    state.localAbsentSince = nil
    state.cloudAbsentSince = nil
  }

  /// A file must stay missing for a while, and in more than one complete snapshot, before its absence is
  /// trusted. iCloud reports a file as removed while it is being replaced or reindexed.
  private func confirmAbsence(_ absentSince: inout TimeInterval?) -> Bool {
    guard let since = absentSince else {
      absentSince = clock.activeTime
      return false
    }
    return clock.activeTime - since >= kAbsenceConfirmationInterval
  }

  /// Remembers the content the app is about to write, so that the same content is not written twice and the
  /// churn it causes on the other side is not mistaken for a user action.
  private func write(_ event: OutgoingSynchronizationEvent,
                     content fingerprint: Fingerprint,
                     to ownedWrite: inout OwnedWrite?) -> [OutgoingSynchronizationEvent] {
    guard ownedWrite?.fingerprint != fingerprint else { return [] }
    ownedWrite = OwnedWrite(fingerprint: fingerprint, startedAt: clock.activeTime)
    return [event]
  }

  private func settle(_ ownedWrite: inout OwnedWrite?, observed fingerprint: Fingerprint) {
    if ownedWrite?.fingerprint == fingerprint {
      ownedWrite = nil
    }
  }

  private func expire(_ ownedWrite: inout OwnedWrite?) {
    if let startedAt = ownedWrite?.startedAt, clock.activeTime - startedAt > Constants.writeSettlingInterval {
      ownedWrite = nil
    }
  }

  private func requestDownload(_ cloudItem: CloudMetadataItem,
                               _ state: inout FileState) -> [OutgoingSynchronizationEvent] {
    shouldRequest(&state.downloadRequestedAt) ? [.startDownloading(cloudItem)] : []
  }

  private func shouldRequest(_ lastRequestTime: inout TimeInterval?) -> Bool {
    if let lastRequestTime, clock.activeTime - lastRequestTime < Constants.requestRepeatInterval {
      return false
    }
    lastRequestTime = clock.activeTime
    return true
  }

  private func finishWriting(_ event: OutgoingSynchronizationEvent, isSuccessful: Bool) {
    guard let fileName = event.fileName else { return }
    mustReconcile = true
    var state = states[fileName] ?? FileState()
    switch event {
    case .createLocalItem, .updateLocalItem:
      // The written content is now the common base of both sides: the destination has just received it and the
      // source had it when the write completed. A change made after that is synchronized as usual.
      complete(&state.ownedLocalWrite, of: fileName, isSuccessful: isSuccessful)
    case .createCloudItem, .updateCloudItem:
      complete(&state.ownedCloudWrite, of: fileName, isSuccessful: isSuccessful)
    case .removeLocalItem, .removeCloudItem:
      cancelAbsences(&state)
      if isSuccessful {
        store.setState(nil, for: fileName)
      }
    case .startDownloading, .resolveVersionsConflict, .didReceiveError:
      break
    }
    states[fileName] = state
  }

  private func complete(_ ownedWrite: inout OwnedWrite?, of fileName: String, isSuccessful: Bool) {
    guard isSuccessful else {
      // A failed write is forgotten so that the same content can be written again.
      ownedWrite = nil
      return
    }
    if let fingerprint = ownedWrite?.fingerprint {
      // The identities are unknown until both directories are observed again.
      store.setState(SynchronizedFileState(fingerprint: fingerprint), for: fileName)
    }
  }
}

// MARK: - SynchronizationClock

/// Time during which synchronization was actually observing both directories. It does not advance in the
/// background, when neither iCloud nor the file system reports anything and nothing can be confirmed.
protocol SynchronizationClock: AnyObject {
  var activeTime: TimeInterval { get }
}

final class ActiveSynchronizationClock: SynchronizationClock {
  private var accumulatedTime: TimeInterval = 0
  private var resumedAt: Date?

  var activeTime: TimeInterval { accumulatedTime - (resumedAt?.timeIntervalSinceNow ?? 0) }

  func resume() {
    if resumedAt == nil {
      resumedAt = Date()
    }
  }

  func pause() {
    accumulatedTime = activeTime
    resumedAt = nil
  }
}
