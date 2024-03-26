typealias MetadataItemName = String
typealias LocalContents = Dictionary<MetadataItemName, LocalMetadataItem>
typealias CloudContents = Dictionary<MetadataItemName, CloudMetadataItem>

protocol SynchronizationStateManager {
  var currentLocalContents: LocalContents { get }
  var currentCloudContents: CloudContents { get }
  var localContentsGatheringIsFinished: Bool { get }
  var cloudContentGatheringIsFinished: Bool { get }

  @discardableResult
  func resolveEvent(_ event: IncomingEvent) -> [OutgoingEvent]
}

enum IncomingEvent {
  case didFinishGatheringLocalContents(LocalContents)
  case didFinishGatheringCloudContents(CloudContents)
  case didUpdateLocalContents(LocalContents)
  case didUpdateCloudContents(CloudContents)
  case didReceiveError(Error)
  case resetState // is used to reset the state of the state manager to initial
}

enum SynchronizationStopReason {
  case userInitiated
  case error(SynchronizationError)
}

enum SynchronizationError: Error {
  case noInternetConnection
  case outOfSpace
  case `internal`(Error)
  // TODO: maybe add another cases
}

enum OutgoingEvent {
  case createLocalItem(CloudMetadataItem)
  case updateLocalItem(CloudMetadataItem)
  case removeLocalItem(CloudMetadataItem)
  case startDownloading(CloudMetadataItem)
  case resolveVersionsConflict(CloudMetadataItem)
  case createCloudItem(LocalMetadataItem)
  case updateCloudItem(LocalMetadataItem)
  case removeCloudItem(LocalMetadataItem)
  case stopSynchronization(SynchronizationStopReason)
  case resumeSynchronization
  case didReceiveError(Error)
}

final class DefaultSynchronizationStateManager: SynchronizationStateManager {

  private(set) var currentLocalContents: LocalContents = [:]
  private(set) var currentCloudContents: CloudContents = [:]
  private(set) var localContentsGatheringIsFinished = false
  private(set) var cloudContentGatheringIsFinished = false

  @discardableResult
  func resolveEvent(_ event: IncomingEvent) -> [OutgoingEvent] {
    let outgoingEvents: [OutgoingEvent]
    switch event {
    case .didFinishGatheringLocalContents(let contents):
      localContentsGatheringIsFinished = true
      outgoingEvents = resolveDidFinishGathering(localContents: contents, cloudContents: currentCloudContents)
    case .didFinishGatheringCloudContents(let contents):
      cloudContentGatheringIsFinished = true
      outgoingEvents = resolveDidFinishGathering(localContents: currentLocalContents, cloudContents: contents)
    case .didUpdateLocalContents(let contents):
      outgoingEvents = resolveDidUpdateLocalContents(contents)
    case .didUpdateCloudContents(let contents):
      outgoingEvents = resolveDidUpdateCloudContents(contents)
    case .didReceiveError(let error):
      outgoingEvents = resolveError(error)
    case .resetState:
      resetState()
      outgoingEvents = []
    }
    return outgoingEvents
  }

  // MARK: - Private
  private func resolveDidFinishGathering(localContents: LocalContents, cloudContents: CloudContents) -> [OutgoingEvent] {
    currentLocalContents = localContents
    currentCloudContents = cloudContents
    guard localContentsGatheringIsFinished, cloudContentGatheringIsFinished else { return [] }

    let outgoingEvents: [OutgoingEvent]
    switch (localContents.isEmpty, cloudContents.isEmpty) {
    case (true, true):
      outgoingEvents = []
    case (true, false):
      outgoingEvents = cloudContents.notInTrash.map { .createLocalItem($0.value) }
    case (false, true):
      outgoingEvents = localContents.map { .createCloudItem($0.value) }
    case (false, false):
      outgoingEvents = resolveDidUpdateCloudContents(cloudContents) + resolveDidUpdateLocalContents(localContents)
    }
    return outgoingEvents
  }

  private func resolveDidUpdateLocalContents(_ localContents: LocalContents) -> [OutgoingEvent] {
    let itemsToCreateInCloud = localContents.reduce(into: LocalContents()) { partialResult, localItem in
      if let cloudItemValue = currentCloudContents[localItem.key] {
        // Merge conflict: if cloud .trash contains item and it's last modification date is less than local item's last modification date than file should be recreated.
        if cloudItemValue.isInTrash, cloudItemValue.lastModificationDate < localItem.value.lastModificationDate {
          partialResult[localItem.key] = localItem.value
        }
      } else {
        partialResult[localItem.key] = localItem.value
      }
    }
    let itemsToRemoveFromCloud = currentLocalContents.filter { !localContents.contains($0.key) }
    let itemsToUpdateInCloud = localContents.reduce(into: LocalContents()) { result, localItem in
      if let cloudItemValue = self.currentCloudContents[localItem.key],
         !cloudItemValue.isInTrash,
         localItem.value.lastModificationDate > cloudItemValue.lastModificationDate {
        result[localItem.key] = localItem.value
      }
    }

    var outgoingEvents = [OutgoingEvent]()
    itemsToCreateInCloud.forEach { outgoingEvents.append(.createCloudItem($0.value)) }
    itemsToRemoveFromCloud.forEach { outgoingEvents.append(.removeCloudItem($0.value)) }
    itemsToUpdateInCloud.forEach { outgoingEvents.append(.updateCloudItem($0.value)) }

    currentLocalContents = localContents
    return outgoingEvents
  }

  private func resolveDidUpdateCloudContents(_ cloudContents: CloudContents) -> [OutgoingEvent] {
    let cloudContentsWithUnresolvedConflicts = cloudContents.notInTrash.withUnresolvedConflicts(true)
    let cloudContentsWithoutUnresolvedConflicts = cloudContents.notInTrash.withUnresolvedConflicts(false)
    let itemsToCreateInLocal = cloudContentsWithoutUnresolvedConflicts.filter { !currentLocalContents.contains($0.key) }
    let itemsToUpdateInLocal = cloudContentsWithoutUnresolvedConflicts.reduce(into: CloudContents()) { result, cloudItem in
      if let localItemValue = self.currentLocalContents[cloudItem.key],
         cloudItem.value.lastModificationDate > localItemValue.lastModificationDate {
        result[cloudItem.key] = cloudItem.value
      }
    }
    let itemsToRemoveFromLocal = cloudContents.trashed.reduce(into: CloudContents()) { result, cloudItem in
      if let localItemValue = self.currentLocalContents[cloudItem.key],
         cloudItem.value.lastModificationDate >= localItemValue.lastModificationDate {
        result[cloudItem.key] = cloudItem.value
      }
    }

    // TODO: Handle situation when file was removed from one storage and updated in another in offline
    var outgoingEvents = [OutgoingEvent]()
    cloudContentsWithUnresolvedConflicts.forEach { outgoingEvents.append(.resolveVersionsConflict($0.value)) }
    itemsToCreateInLocal.notDownloaded.forEach { outgoingEvents.append(.startDownloading($0.value)) }
    itemsToUpdateInLocal.notDownloaded.forEach { outgoingEvents.append(.startDownloading($0.value)) }
    itemsToCreateInLocal.downloaded.forEach { outgoingEvents.append(.createLocalItem($0.value)) }
    itemsToUpdateInLocal.downloaded.forEach { outgoingEvents.append(.updateLocalItem($0.value)) }
    itemsToRemoveFromLocal.forEach { outgoingEvents.append(.removeLocalItem($0.value)) }

    currentCloudContents = cloudContents
    return outgoingEvents
  }

  private func resolveError(_ error: Error) -> [OutgoingEvent] {
    // TODO: handle error to parse it to OutgoingEvent.stopSynchronization
    return [.didReceiveError(error)]
  }

  private func resetState() {
    currentLocalContents.removeAll()
    currentCloudContents.removeAll()
    localContentsGatheringIsFinished = false
    cloudContentGatheringIsFinished = false
  }
}

// MARK: - MetadataItem Dictionary + Contains
extension Dictionary where Key == MetadataItemName, Value: MetadataItem {
  func contains(_ item: Key) -> Bool {
    return keys.contains(item)
  }

  mutating func add(_ item: Value) {
    self[item.fileName] = item
  }

  init(_ items: [Value]) {
    self.init(uniqueKeysWithValues: items.map { ($0.fileName, $0) })
  }
}

// MARK: - CloudMetadataItem Dictionary + Trash, Down
extension Dictionary where Key == MetadataItemName, Value == CloudMetadataItem {
  var trashed: Self {
    return filter { $0.value.isInTrash }
  }

  var notInTrash: Self {
    return filter { !$0.value.isInTrash }
  }

  var downloaded: Self {
    return filter { $0.value.isDownloaded }
  }

  var notDownloaded: Self {
    return filter { !$0.value.isDownloaded }
  }

  func withUnresolvedConflicts(_ hasUnresolvedConflicts: Bool) -> Self {
    return filter { $0.value.hasUnresolvedConflicts == hasUnresolvedConflicts }
  }
}
