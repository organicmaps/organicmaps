typealias LocalContents = [LocalMetadataItem]
typealias CloudContents = [CloudMetadataItem]
typealias LocalSnapshot = DirectorySnapshot<LocalMetadataItem>
typealias CloudSnapshot = DirectorySnapshot<CloudMetadataItem>

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

/// What made a deletion legitimate, carried by the event that crosses the queues. Both the absence that was
/// confirmed and the content the surviving copy is expected to hold are checked again right before the file is
/// deleted: a file that disappeared, came back and disappeared again is missing for a reason nobody confirmed.
struct DeletionEvidence: Equatable {
  /// When the confirmed absence started, in active synchronization time.
  let absentSince: TimeInterval
  /// The content both sides held when they were last synchronized.
  let base: Fingerprint
}

enum IncomingSynchronizationEvent {
  case didUpdateLocalContents(LocalSnapshot)
  case didUpdateCloudContents(CloudSnapshot)
  case didFinishWriting(OutgoingSynchronizationEvent)
  case didFailWriting(OutgoingSynchronizationEvent)
  /// The directories did not change, but what is known about their content did: a file that could not be
  /// compared before has been read, or one that could not be read at all may be readable now.
  case didUpdateKnownContents
}

enum OutgoingSynchronizationEvent: Equatable {
  case startDownloading(CloudMetadataItem)

  case createLocalItem(with: CloudMetadataItem)
  /// Replaces the local file with the cloud one. When the local version holds changes that were never
  /// synchronized, it is the only copy of them and is preserved under a new name by the same operation.
  case updateLocalItem(with: CloudMetadataItem, preserving: LocalMetadataItem?)
  case removeLocalItem(LocalMetadataItem, DeletionEvidence)

  case createCloudItem(with: LocalMetadataItem)
  case updateCloudItem(with: LocalMetadataItem)
  case removeCloudItem(CloudMetadataItem, DeletionEvidence)

  case resolveVersionsConflict(CloudMetadataItem)

  var fileName: String {
    switch self {
    case .startDownloading(let item), .createLocalItem(let item), .updateLocalItem(let item, _),
         .removeCloudItem(let item, _), .resolveVersionsConflict(let item):
      return item.fileName
    case .createCloudItem(let item), .updateCloudItem(let item), .removeLocalItem(let item, _):
      return item.fileName
    }
  }
}

/// Reconciles the local and the cloud directory file by file.
///
/// Every incoming snapshot is an observation, not a command: the resolver keeps the state of each file and
/// derives what has to be written from the content of both sides, comparing them with the content that was
/// last synchronized. Deletions are the only irreversible operation and require a confirmed absence.
final class iCloudSynchronizationStateResolver: SynchronizationStateResolver {
  enum Constants {
    /// How long a file must stay missing, in active synchronization time, before it is deleted on the other
    /// side. The directories are observed anew after this interval: a file that stays missing is reported by
    /// nobody.
    static let absenceConfirmationInterval: TimeInterval = 30
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

  /// A file that is missing from one of the directories. Only a later snapshot of that same directory can
  /// confirm the absence, so the one that reported it is a part of it.
  private struct Absence {
    /// When the file was first seen missing, in active synchronization time.
    let since: TimeInterval
    /// The number of the snapshot of the missing side that reported the file as gone.
    let observedIn: Int
  }

  private struct FileState {
    var ownedLocalWrite: OwnedWrite?
    var ownedCloudWrite: OwnedWrite?
    var localAbsence: Absence?
    var cloudAbsence: Absence?
    /// A deletion is irreversible and is requested once, however many snapshots confirm the same absence while
    /// the request is on its way. Cancelling the absence -- which every result of the deletion does -- requests
    /// it again after a fresh confirmation.
    var deletionRequested = false
    var downloadRequestedAt: TimeInterval?
    var conflictResolutionRequestedAt: TimeInterval?

    /// True while the file waits for something that only a new snapshot can confirm: an absence that has to be
    /// observed again, a write that has to settle, or a request iCloud has not acted on yet. Without this the
    /// repeated snapshots are skipped, and a request that iCloud ignores is never repeated.
    var isPending: Bool {
      localAbsence != nil || cloudAbsence != nil || ownedLocalWrite != nil || ownedCloudWrite != nil
        || downloadRequestedAt != nil || conflictResolutionRequestedAt != nil
    }
  }

  private let store: SynchronizedStateStore
  private let fingerprintProvider: FingerprintProvider
  private let clock: SynchronizationClock
  private var states = [String: FileState]()
  private var localSnapshot: LocalSnapshot?
  private var cloudSnapshot: CloudSnapshot?
  /// How many times each directory has been observed. An absence is trusted only after the directory it is
  /// missing from has been looked at again, and a snapshot that repeats the previous one is such a look too.
  private var localSnapshotNumber = 0
  private var cloudSnapshotNumber = 0
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
      localSnapshotNumber += 1
      // iCloud repeats a snapshot on every uploading and downloading step: reconcile it only when it brings
      // something new or when a file still waits for something.
      guard snapshot != localSnapshot || mustReconcile || hasPendingConfirmations else { return [] }
      localSnapshot = snapshot
    case .didUpdateCloudContents(let snapshot):
      cloudSnapshotNumber += 1
      guard snapshot != cloudSnapshot || mustReconcile || hasPendingConfirmations else { return [] }
      cloudSnapshot = snapshot
    case .didFinishWriting(let event):
      finishWriting(event, isSuccessful: true)
      return []
    case .didFailWriting(let event):
      finishWriting(event, isSuccessful: false)
      return []
    case .didUpdateKnownContents:
      break
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
    case .removeLocalItem(let item, let evidence):
      return isDeletionConfirmed(states[item.fileName]?.cloudAbsence,
                                 observedIn: cloudSnapshotNumber,
                                 evidence,
                                 of: item.fileName)
    case .removeCloudItem(let item, let evidence):
      return isDeletionConfirmed(states[item.fileName]?.localAbsence,
                                 observedIn: localSnapshotNumber,
                                 evidence,
                                 of: item.fileName)
    default:
      return true
    }
  }

  func resetState() {
    LOG(.debug, "Resetting state")
    states.removeAll()
    localSnapshot = nil
    cloudSnapshot = nil
    localSnapshotNumber = 0
    cloudSnapshotNumber = 0
    mustReconcile = true
  }

  // MARK: - Reconciliation

  private func reconcile() -> [OutgoingSynchronizationEvent] {
    // Both directories must be observed at least once: an unobserved directory looks empty.
    guard let localSnapshot, let cloudSnapshot else { return [] }
    mustReconcile = false

    var events = [OutgoingSynchronizationEvent]()
    for fileName in localSnapshot.fileNames.union(cloudSnapshot.fileNames).union(states.keys) {
      let local = localSnapshot.state(of: fileName)
      let cloud = cloudSnapshot.state(of: fileName)
      var state = states[fileName] ?? FileState()
      expire(&state.ownedLocalWrite)
      expire(&state.ownedCloudWrite)
      events.append(contentsOf: resolve(local: local, cloud: cloud, state: &state))
      if case .absent = local, case .absent = cloud {
        // Nobody holds the file anymore: there is nothing left to compare it against and nothing to wait for.
        store.setState(nil, for: fileName)
        states.removeValue(forKey: fileName)
      } else {
        states[fileName] = state
      }
    }
    return events
  }

  private func resolve(local: LocalSnapshot.ItemState,
                       cloud: CloudSnapshot.ItemState,
                       state: inout FileState) -> [OutgoingSynchronizationEvent] {
    if case .present(let cloudItem) = cloud, cloudItem.hasUnresolvedConflicts {
      cancelAbsences(&state)
      return shouldRequest(&state.conflictResolutionRequestedAt) ? [.resolveVersionsConflict(cloudItem)] : []
    }
    state.conflictResolutionRequestedAt = nil

    switch (local, cloud) {
    case (.present(let localItem), .present(let cloudItem)):
      cancelAbsences(&state)
      return resolvePresentOnBothSides(localItem, cloudItem, &state)
    case (.present(let localItem), .absent):
      state.localAbsence = nil
      return resolveMissingCloudFile(localItem, &state)
    case (.absent, .present(let cloudItem)):
      state.cloudAbsence = nil
      return resolveMissingLocalFile(cloudItem, &state)
    case (.absent, .absent):
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
    /* Both sides are asked before anything is concluded, so that the two reads start in one round instead of
     one per report. A content that is not known yet is being read in the background, or the file cannot be read
     at all while iCloud replaces it: the provider reports when it is worth asking again. */
    let localFingerprint = fingerprintProvider.fingerprint(of: localItem)
    let cloudFingerprint = fingerprintProvider.fingerprint(of: cloudItem)
    guard let localFingerprint, let cloudFingerprint else { return [] }

    settle(&state.ownedLocalWrite, observed: localFingerprint)
    settle(&state.ownedCloudWrite, observed: cloudFingerprint)

    guard localFingerprint != cloudFingerprint else {
      store.setState(SynchronizedFileState(fingerprint: localFingerprint,
                                           localIdentity: localItem.identity,
                                           cloudIdentity: cloudItem.identity),
                     for: fileName)
      return []
    }
    /* A side the app is still writing to may be observed as it was before that write: iCloud reports the
     attributes of a file it is replacing with a delay, and a file that still looks the same is not read again.
     What is seen there proves nothing until the write settles. */
    if synchronized?.fingerprint == localFingerprint {
      guard state.ownedCloudWrite == nil else { return [] }
      return write(.updateLocalItem(with: cloudItem, preserving: nil), content: cloudFingerprint, to: &state.ownedLocalWrite)
    }
    if synchronized?.fingerprint == cloudFingerprint {
      guard state.ownedLocalWrite == nil else { return [] }
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
      state.cloudAbsence = nil
      return write(.createCloudItem(with: localItem), content: localFingerprint, to: &state.ownedCloudWrite)
    }
    guard state.ownedCloudWrite == nil else {
      // iCloud removes the file while it replaces it. Our own write is not a deletion.
      state.cloudAbsence = nil
      return []
    }
    guard let absentSince = confirmAbsence(&state.cloudAbsence, observedIn: cloudSnapshotNumber),
          !state.deletionRequested
    else { return [] }
    state.deletionRequested = true
    return [.removeLocalItem(localItem, DeletionEvidence(absentSince: absentSince, base: localFingerprint))]
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
      state.localAbsence = nil
      return write(.createLocalItem(with: cloudItem), content: cloudFingerprint, to: &state.ownedLocalWrite)
    }
    guard state.ownedLocalWrite == nil else {
      state.localAbsence = nil
      return []
    }
    guard let absentSince = confirmAbsence(&state.localAbsence, observedIn: localSnapshotNumber),
          !state.deletionRequested
    else { return [] }
    state.deletionRequested = true
    return [.removeCloudItem(cloudItem, DeletionEvidence(absentSince: absentSince, base: cloudFingerprint))]
  }

  // MARK: - File state

  private func cancelAbsences(_ state: inout FileState) {
    state.localAbsence = nil
    state.cloudAbsence = nil
    state.deletionRequested = false
  }

  /// A file must stay missing for a while, and in more than one complete snapshot of the directory it is missing
  /// from, before its absence is trusted: iCloud reports a file as removed while it is being replaced or
  /// reindexed. Returns when the confirmed absence started, or nil while it is not confirmed yet.
  private func confirmAbsence(_ absence: inout Absence?, observedIn snapshotNumber: Int) -> TimeInterval? {
    guard let started = absence else {
      absence = Absence(since: clock.activeTime, observedIn: snapshotNumber)
      return nil
    }
    return isAbsenceConfirmed(started, observedIn: snapshotNumber) ? started.since : nil
  }

  /// The snapshot that started the absence confirms nothing more than it reported: the directory the file is
  /// missing from has to be observed again, and the file has to be missing from that observation too.
  private func isAbsenceConfirmed(_ absence: Absence?, observedIn snapshotNumber: Int) -> Bool {
    guard let absence else { return false }
    return snapshotNumber > absence.observedIn
      && clock.activeTime - absence.since >= Constants.absenceConfirmationInterval
  }

  /// Exactly the rule that produced the deletion: the very absence that was confirmed still stands -- an absence
  /// that was cancelled and started anew was never confirmed -- and the copy that survives still holds the
  /// content that was last synchronized, so nothing changed there while the event was crossing the queues.
  private func isDeletionConfirmed(_ absence: Absence?,
                                   observedIn snapshotNumber: Int,
                                   _ evidence: DeletionEvidence,
                                   of fileName: String) -> Bool {
    absence?.since == evidence.absentSince && isAbsenceConfirmed(absence, observedIn: snapshotNumber)
      && store.state(for: fileName)?.fingerprint == evidence.base
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
    let fileName = event.fileName
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
      /* The content that was last synchronized is kept until both directories report the file as gone. A
       deletion that reported success may still have done nothing -- the category was not loaded, or its file
       could not be moved to the trash -- and forgetting the common base here would turn the file that is still
       on disk into a new one to upload. */
      cancelAbsences(&state)
    case .startDownloading, .resolveVersionsConflict:
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

/// Measured with the system uptime and not with the wall clock: a clock moved forward -- by the user, by the
/// network -- would instantly satisfy the interval a deletion has to wait for. Uptime does not advance while the
/// device sleeps, and neither does synchronization, which observes nothing then.
final class ActiveSynchronizationClock: SynchronizationClock {
  private var accumulatedTime: TimeInterval = 0
  private var resumedAt: TimeInterval?

  var activeTime: TimeInterval {
    accumulatedTime + (resumedAt.map { ProcessInfo.processInfo.systemUptime - $0 } ?? 0)
  }

  func resume() {
    if resumedAt == nil {
      resumedAt = ProcessInfo.processInfo.systemUptime
    }
  }

  func pause() {
    accumulatedTime = activeTime
    resumedAt = nil
  }
}
