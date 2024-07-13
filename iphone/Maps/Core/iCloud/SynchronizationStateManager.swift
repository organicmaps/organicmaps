typealias LocalContentsMetadata = [LocalMetadataItem]
typealias CloudContentsMetadata = [CloudMetadataItem]

struct DirectoryContentsMetadata<T: MetadataItem> {
  var deleted: [T]
  var notDeleted: [T]
  var removed: [T]
}

protocol SynchronizationStateManager {
  var localContents: DirectoryContentsMetadata<LocalMetadataItem> { get }
  var cloudContents: DirectoryContentsMetadata<CloudMetadataItem> { get }
  var localContentsGatheringIsFinished: Bool { get }
  var cloudContentGatheringIsFinished: Bool { get }

  @discardableResult
  func resolveEvent(_ event: IncomingCloudEvent) -> [OutgoingCloudEvent]
  func resetState()
}

enum IncomingCloudEvent {
  case didFinishGatheringLocalContents(LocalContentsMetadata)
  case didFinishGatheringCloudContents(CloudContentsMetadata)
  case didUpdateLocalContents(LocalContentsMetadata)
  case didUpdateCloudContents(CloudContentsMetadata)
}

enum OutgoingCloudEvent {
  case createLocalItem(from: CloudMetadataItem)
  case updateLocalItem(from: CloudMetadataItem)
  case removeLocalItem(LocalMetadataItem)
  case startDownloading(CloudMetadataItem)
  case createCloudItem(from: LocalMetadataItem)
  case updateCloudItem(from: LocalMetadataItem)
  case removeCloudItem(CloudMetadataItem)
  case trashCloudItem(LocalMetadataItem)
  case didReceiveError(SynchronizationError)
  case resolveVersionsConflict(CloudMetadataItem)
  case resolveInitialSynchronizationConflict(LocalMetadataItem)
  case didFinishInitialSynchronization
}

final class CloudSynchronizationStateManager: SynchronizationStateManager {

  private enum SynchronizationStrategy: Int, Equatable {
    case initial
    case `default`
  }

  // MARK: - Public properties
  private(set) var localContents = DirectoryContentsMetadata<LocalMetadataItem>()
  private(set) var cloudContents = DirectoryContentsMetadata<CloudMetadataItem>()

  private(set) var localContentsGatheringIsFinished = false
  private(set) var cloudContentGatheringIsFinished = false

  private var strategy: SynchronizationStrategy

  init(isInitialSynchronization: Bool) {
    strategy = isInitialSynchronization ? .initial : .default
  }

  // MARK: - Public methods
  @discardableResult
  func resolveEvent(_ event: IncomingCloudEvent) -> [OutgoingCloudEvent] {
    let outgoingEvents: [OutgoingCloudEvent]
    switch event {
    case .didFinishGatheringLocalContents(let localContents):
      localContentsGatheringIsFinished = true
      outgoingEvents = resolveDidFinishGathering(localContents: localContents, cloudContents: cloudContents.notRemoved)
    case .didFinishGatheringCloudContents(let cloudContents):
      cloudContentGatheringIsFinished = true
      outgoingEvents = resolveDidFinishGathering(localContents: localContents.notRemoved, cloudContents: cloudContents)
    case .didUpdateLocalContents(let contents):
      outgoingEvents = resolveDidUpdateLocalContents(contents)
    case .didUpdateCloudContents(let contents):
      outgoingEvents = resolveDidUpdateCloudContents(contents)
    }
    return outgoingEvents
  }

  func resetState() {
    localContents.removeAll()
    cloudContents.removeAll()
    localContentsGatheringIsFinished = false
    cloudContentGatheringIsFinished = false
  }

  // MARK: - Private methods
  private func updateLocalContents(_ contents: LocalContentsMetadata) {
    localContents.deleted = contents.filter { $0.isDeleted }
    localContents.notDeleted = contents.filter { !$0.isDeleted }
    localContents.removed = []
  }

  private func updateCloudContents(_ contents: CloudContentsMetadata) {
    cloudContents.deleted = contents.filter { !$0.isRemoved && $0.isDeleted }
    cloudContents.notDeleted = contents.filter { !$0.isRemoved && !$0.isDeleted }
    cloudContents.removed = contents.filter { $0.isRemoved }
  }

  private func resolveDidFinishGathering(localContents: LocalContentsMetadata, cloudContents: CloudContentsMetadata) -> [OutgoingCloudEvent] {
    updateLocalContents(localContents)
    updateCloudContents(cloudContents)
    guard localContentsGatheringIsFinished, cloudContentGatheringIsFinished else { return [] }

    var outgoingEvents: [OutgoingCloudEvent]
    switch (localContents.isEmpty, cloudContents.isEmpty) {
    case (true, true):
      outgoingEvents = []
    case (true, false):
      outgoingEvents = self.cloudContents.notDeleted.map { .createLocalItem(from: $0) }
    case (false, true):
      outgoingEvents = localContents.map { .createCloudItem(from: $0) }
    case (false, false):
      var events = [OutgoingCloudEvent]()
      if strategy == .initial {
        events.append(contentsOf: resolveInitialSynchronizationConflicts(localContents: localContents, cloudContents: cloudContents))
      }
      events.append(contentsOf: resolveDidUpdateCloudContents(cloudContents))
      events.append(contentsOf: resolveDidUpdateLocalContents(localContents))
      outgoingEvents = events
    }
    if strategy == .initial {
      outgoingEvents.append(.didFinishInitialSynchronization)
    }
    strategy = .default
    return outgoingEvents
  }

  private func resolveDidUpdateLocalContents(_ localContents: LocalContentsMetadata) -> [OutgoingCloudEvent] {
    let previousLocalContent = self.localContents
    updateLocalContents(localContents)

    let itemsToTrashFromCloudContainer = getItemsToTrashFromCloudContainer(previousLocalContent: previousLocalContent)
    let itemsToRemoveFromCloudContainer = getItemsToRemoveFromCloudContainer()
    let itemsToCreateInCloudContainer = getItemsToCreateInCloudContainer()
    let itemsToUpdateInCloudContainer = getItemsToUpdateInCloudContainer()

    var outgoingEvents = [OutgoingCloudEvent]()
    itemsToRemoveFromCloudContainer.forEach { outgoingEvents.append(.removeCloudItem($0)) }
    itemsToTrashFromCloudContainer.forEach { outgoingEvents.append(.trashCloudItem($0)) }
    itemsToCreateInCloudContainer.forEach { outgoingEvents.append(.createCloudItem(from: $0)) }
    itemsToUpdateInCloudContainer.forEach { outgoingEvents.append(.updateCloudItem(from: $0)) }

    return outgoingEvents
  }

  private func resolveDidUpdateCloudContents(_ cloudContents: CloudContentsMetadata) -> [OutgoingCloudEvent] {
    var outgoingEvents = [OutgoingCloudEvent]()
    updateCloudContents(cloudContents)

    let errors = getItemsWithErrors(cloudContents)
    errors.forEach { outgoingEvents.append(.didReceiveError($0)) }

    let itemsWithUnresolvedConflicts = getItemsToResolveConflicts()
    itemsWithUnresolvedConflicts.forEach { outgoingEvents.append(.resolveVersionsConflict($0)) }

    // Merge conflicts and errors should be resolved before handling other events.
    guard outgoingEvents.isEmpty else {
      return outgoingEvents
    }

    let itemsToRemoveFromLocalContainer = getItemsToRemoveFromLocalContainer()
    let itemsToCreateInLocalContainer = getItemsToCreateInLocalContainer()
    let itemsToUpdateInLocalContainer = getItemsToUpdateInLocalContainer()

    itemsToCreateInLocalContainer.notDownloaded.forEach { outgoingEvents.append(.startDownloading($0)) }
    itemsToUpdateInLocalContainer.notDownloaded.forEach { outgoingEvents.append(.startDownloading($0)) }

    // Download the files before merging them.
    guard outgoingEvents.isEmpty else {
      return outgoingEvents
    }

    itemsToRemoveFromLocalContainer.forEach { outgoingEvents.append(.removeLocalItem($0)) }
    itemsToCreateInLocalContainer.downloaded.forEach { outgoingEvents.append(.createLocalItem(from: $0)) }
    itemsToUpdateInLocalContainer.downloaded.forEach { outgoingEvents.append(.updateLocalItem(from: $0)) }

    return outgoingEvents
  }

  // MARK: - Cloud

  private func getItemsToRemoveFromCloudContainer() -> CloudContentsMetadata {
    let deletedItemsToRemove = cloudContents.notDeleted.filter { cloudItem in
      if let deletedLocalItem = localContents.deleted.firstByNameWithoutExtension(cloudItem),
         deletedLocalItem.lastModificationDate > cloudItem.lastModificationDate {
        return true
      }
      return false
    }
    let recoveredItemsToRemove = cloudContents.deleted.filter { deletedCloudItem in
      if let localItem = localContents.notDeleted.firstByNameWithoutExtension(deletedCloudItem),
         localItem.lastModificationDate > deletedCloudItem.lastModificationDate {
        return true
      }
      return false
    }
    return deletedItemsToRemove + recoveredItemsToRemove
  }

  private func getItemsToTrashFromCloudContainer(previousLocalContent: DirectoryContentsMetadata<LocalMetadataItem>) -> LocalContentsMetadata {
    // Trashing the items to the Cloud's Trash is only happens due to users's manual action to delete the recently deleted file. previousLocalContent used to handle this deletion action.
    previousLocalContent.deleted.filter { !localContents.deleted.containsByNameWithoutExtension($0) && !localContents.notDeleted.containsByNameWithoutExtension($0) }
  }

  private func getItemsToCreateInCloudContainer() -> LocalContentsMetadata {
    localContents.notRemoved.filter { localItem in
      guard !cloudContents.notRemoved.containsByName(localItem) else {
        return false
      }
      if let deletedCloudItem = cloudContents.deleted.firstByNameWithoutExtension(localItem),
         deletedCloudItem.lastModificationDate > localItem.lastModificationDate {
        return false
      }
      if let сloudItem = cloudContents.notDeleted.firstByNameWithoutExtension(localItem),
         сloudItem.lastModificationDate > localItem.lastModificationDate {
        return false
      }
      return true
    }
  }

  private func getItemsToUpdateInCloudContainer() -> LocalContentsMetadata {
    guard strategy != .initial else { return [] }
    // Due to the initial sync all conflicted local items will be duplicated with different name and replaced by the cloud items to avoid a data loss.
    return localContents.notRemoved.filter { localItem in
      if let cloudItem = cloudContents.notRemoved.firstByName(localItem),
         localItem.lastModificationDate > cloudItem.lastModificationDate {
        return true
      }
      return false
    }
  }

  // MARK: - Local

  private func getItemsToRemoveFromLocalContainer() -> LocalContentsMetadata {
    let deletedItemsToRemove = cloudContents.deleted.reduce(into: LocalContentsMetadata(), { partialResult, deletedCloudItem in
      if let localItem = localContents.notDeleted.firstByNameWithoutExtension(deletedCloudItem),
         deletedCloudItem.lastModificationDate > localItem.lastModificationDate {
        partialResult.append(localItem)
      }
    })
    let recoveredItemsToRemove = cloudContents.notDeleted.reduce(into: LocalContentsMetadata(), { partialResult, cloudItem in
      if let localItem = localContents.deleted.firstByNameWithoutExtension(cloudItem),
         cloudItem.lastModificationDate > localItem.lastModificationDate {
        partialResult.append(localItem)
      }
    })
    let removedItemsToRemove = cloudContents.removed.reduce(into: LocalContentsMetadata(), { partialResult, removedCloudItem in
      if let localDeletedItem = localContents.deleted.firstByName(removedCloudItem),
         removedCloudItem.lastModificationDate == localDeletedItem.lastModificationDate {
        partialResult.append(localDeletedItem)
      }
      if let localItem = localContents.notDeleted.firstByName(removedCloudItem),
         removedCloudItem.lastModificationDate == localItem.lastModificationDate {
        partialResult.append(localItem)
      }
    })
    return deletedItemsToRemove + recoveredItemsToRemove + removedItemsToRemove
  }

  private func getItemsToCreateInLocalContainer() -> CloudContentsMetadata {
    let deletedItemsToCreate = cloudContents.deleted.filter { deletedCloudItem in
      if !localContents.deleted.containsByName(deletedCloudItem) {
        return true
      }
      if let localItem = localContents.notDeleted.firstByNameWithoutExtension(deletedCloudItem),
         deletedCloudItem.lastModificationDate > localItem.lastModificationDate {
        return true
      }
      return false
    }
    let itemsToCreate = cloudContents.notDeleted.filter { cloudItem in
      if !localContents.notDeleted.containsByName(cloudItem) {
        return true
      }
      if let localItem = localContents.deleted.firstByNameWithoutExtension(cloudItem),
         cloudItem.lastModificationDate > localItem.lastModificationDate {
        return true
      }
      return false
    }
    return deletedItemsToCreate + itemsToCreate
  }

  private func getItemsToUpdateInLocalContainer() -> CloudContentsMetadata {
    cloudContents.notRemoved.withUnresolvedConflicts(false).reduce(into: CloudContentsMetadata()) { result, cloudItem in
      if let localItemValue = localContents.notDeleted.firstByName(cloudItem) {
        // Due to the initial sync all conflicted local items will be duplicated with different name and replaced by the cloud items to avoid a data loss.
        if strategy == .initial {
          result.append(cloudItem)
        } else if cloudItem.lastModificationDate > localItemValue.lastModificationDate {
          result.append(cloudItem)
        }
      }
    }
  }

  // MARK: - Errors and conflicts

  private func resolveInitialSynchronizationConflicts(localContents: LocalContentsMetadata, cloudContents: CloudContentsMetadata) -> [OutgoingCloudEvent] {
    let itemsInInitialConflict = localContents.filter { cloudContents.containsByName($0) }
    guard !itemsInInitialConflict.isEmpty else {
      return []
    }
    return itemsInInitialConflict.map { .resolveInitialSynchronizationConflict($0) }
  }

  private func getItemsWithErrors(_ cloudContents: CloudContentsMetadata) -> [SynchronizationError] {
    cloudContents.reduce(into: [SynchronizationError](), { partialResult, cloudItem in
      if let downloadingError = cloudItem.downloadingError, let synchronizationError = SynchronizationError.fromError(downloadingError) {
        partialResult.append(synchronizationError)
      }
      if let uploadingError = cloudItem.uploadingError, let synchronizationError = SynchronizationError.fromError(uploadingError) {
        partialResult.append(synchronizationError)
      }
    })
  }

  private func getItemsToResolveConflicts() -> CloudContentsMetadata {
    cloudContents.notDeleted.withUnresolvedConflicts(true)
  }
}

private extension DirectoryContentsMetadata {
  init() {
    self.init(deleted: [], notDeleted: [], removed: [])
  }

  mutating func removeAll() {
    deleted.removeAll()
    notDeleted.removeAll()
  }

  var notRemoved: [T] {
    deleted + notDeleted
  }
}
