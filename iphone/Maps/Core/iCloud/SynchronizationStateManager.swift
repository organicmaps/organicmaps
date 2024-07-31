typealias MetadataItemName = String
typealias LocalContents = [LocalMetadataItem]
typealias CloudContents = [CloudMetadataItem]

protocol SynchronizationStateManager {
  var currentLocalContents: LocalContents { get }
  var currentCloudContents: CloudContents { get }
  var localContentsGatheringIsFinished: Bool { get }
  var cloudContentGatheringIsFinished: Bool { get }

  @discardableResult
  func resolveEvent(_ event: IncomingEvent) -> [OutgoingEvent]
  func resetState()
}

enum IncomingEvent {
  case didFinishGatheringLocalContents(LocalContents)
  case didFinishGatheringCloudContents(CloudContents)
  case didUpdateLocalContents(LocalContents)
  case didUpdateCloudContents(CloudContents)
}

enum OutgoingEvent {
  case createLocalItem(CloudMetadataItem)
  case updateLocalItem(CloudMetadataItem)
  case removeLocalItem(CloudMetadataItem)
  case startDownloading(CloudMetadataItem)
  case createCloudItem(LocalMetadataItem)
  case updateCloudItem(LocalMetadataItem)
  case removeCloudItem(LocalMetadataItem)
  case didReceiveError(SynchronizationError)
  case resolveVersionsConflict(CloudMetadataItem)
  case resolveInitialSynchronizationConflict(LocalMetadataItem)
  case didFinishInitialSynchronization
}

final class DefaultSynchronizationStateManager: SynchronizationStateManager {

  // MARK: - Public properties
  private(set) var localContentsGatheringIsFinished = false
  private(set) var cloudContentGatheringIsFinished = false

  private(set) var currentLocalContents: LocalContents = []
  private(set) var currentCloudContents: CloudContents = [] {
    didSet {
      updateFilteredCloudContents()
    }
  }

  // Cached derived arrays
  private var trashedCloudContents: [CloudMetadataItem] = []
  private var notTrashedCloudContents: [CloudMetadataItem] = []
  private var downloadedCloudContents: [CloudMetadataItem] = []
  private var notDownloadedCloudContents: [CloudMetadataItem] = []

  private var isInitialSynchronization: Bool

  init(isInitialSynchronization: Bool) {
    self.isInitialSynchronization = isInitialSynchronization
  }

  // MARK: - Public methods
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
    }
    LOG(.info, "Cloud content: \n\(currentCloudContents.shortDebugDescription)")
    LOG(.info, "Local content: \n\(currentLocalContents.shortDebugDescription)")
    LOG(.info, "Events to process: \n\(outgoingEvents)")
    return outgoingEvents
  }

  func resetState() {
    LOG(.debug, "Resetting state")
    currentLocalContents.removeAll()
    currentCloudContents.removeAll()
    localContentsGatheringIsFinished = false
    cloudContentGatheringIsFinished = false
  }

  // MARK: - Private methods
  private func updateFilteredCloudContents() {
    trashedCloudContents = currentCloudContents.filter { $0.isRemoved }
    notTrashedCloudContents = currentCloudContents.filter { !$0.isRemoved }
  }

  private func resolveDidFinishGathering(localContents: LocalContents, cloudContents: CloudContents) -> [OutgoingEvent] {
    currentLocalContents = localContents
    currentCloudContents = cloudContents
    guard localContentsGatheringIsFinished, cloudContentGatheringIsFinished else { return [] }
    
    var outgoingEvents: [OutgoingEvent]
    switch (localContents.isEmpty, cloudContents.isEmpty) {
    case (true, true):
      outgoingEvents = []
    case (true, false):
      outgoingEvents = notTrashedCloudContents.map { .createLocalItem($0) }
    case (false, true):
      outgoingEvents = localContents.map { .createCloudItem($0) }
    case (false, false):
      var events = [OutgoingEvent]()
      if isInitialSynchronization {
        events.append(contentsOf: resolveInitialSynchronizationConflicts(localContents: localContents, cloudContents: cloudContents))
      }
      events.append(contentsOf: resolveDidUpdateCloudContents(cloudContents))
      events.append(contentsOf: resolveDidUpdateLocalContents(localContents))
      outgoingEvents = events
    }
    if isInitialSynchronization {
      outgoingEvents.append(.didFinishInitialSynchronization)
      isInitialSynchronization = false
    }
    return outgoingEvents
  }

  private func resolveDidUpdateLocalContents(_ localContents: LocalContents) -> [OutgoingEvent] {
    let itemsToRemoveFromCloudContainer = Self.getItemsToRemoveFromCloudContainer(currentLocalContents: currentLocalContents, 
                                                                                  newLocalContents: localContents)
    let itemsToCreateInCloudContainer = Self.getItemsToCreateInCloudContainer(notTrashedCloudContents: notTrashedCloudContents, 
                                                                              trashedCloudContents: trashedCloudContents,
                                                                              localContents: localContents)
    let itemsToUpdateInCloudContainer = Self.getItemsToUpdateInCloudContainer(notTrashedCloudContents: notTrashedCloudContents,
                                                                              localContents: localContents,
                                                                              isInitialSynchronization: isInitialSynchronization)

    var outgoingEvents = [OutgoingEvent]()
    itemsToRemoveFromCloudContainer.forEach { outgoingEvents.append(.removeCloudItem($0)) }
    itemsToCreateInCloudContainer.forEach { outgoingEvents.append(.createCloudItem($0)) }
    itemsToUpdateInCloudContainer.forEach { outgoingEvents.append(.updateCloudItem($0)) }

    currentLocalContents = localContents
    return outgoingEvents
  }

  private func resolveDidUpdateCloudContents(_ cloudContents: CloudContents) -> [OutgoingEvent] {
    var outgoingEvents = [OutgoingEvent]()
    currentCloudContents = cloudContents

    // 1. Handle errors
    let errors = Self.getItemsWithErrors(cloudContents)
    errors.forEach { outgoingEvents.append(.didReceiveError($0)) }

    // 2. Handle merge conflicts
    let itemsWithUnresolvedConflicts = Self.getItemsToResolveConflicts(notTrashedCloudContents: notTrashedCloudContents)
    itemsWithUnresolvedConflicts.forEach { outgoingEvents.append(.resolveVersionsConflict($0)) }

    // Merge conflicts should be resolved at first.
    guard itemsWithUnresolvedConflicts.isEmpty else {
      return outgoingEvents
    }

    let itemsToRemoveFromLocalContainer = Self.getItemsToRemoveFromLocalContainer(notTrashedCloudContents: notTrashedCloudContents, 
                                                                                  trashedCloudContents: trashedCloudContents,
                                                                                  localContents: currentLocalContents)
    let itemsToCreateInLocalContainer = Self.getItemsToCreateInLocalContainer(notTrashedCloudContents: notTrashedCloudContents, 
                                                                              localContents: currentLocalContents)
    let itemsToUpdateInLocalContainer = Self.getItemsToUpdateInLocalContainer(notTrashedCloudContents: notTrashedCloudContents, 
                                                                              localContents: currentLocalContents,
                                                                              isInitialSynchronization: isInitialSynchronization)

    // 3. Handle not downloaded items
    itemsToCreateInLocalContainer.notDownloaded.forEach { outgoingEvents.append(.startDownloading($0)) }
    itemsToUpdateInLocalContainer.notDownloaded.forEach { outgoingEvents.append(.startDownloading($0)) }

    // 4. Handle downloaded items
    itemsToRemoveFromLocalContainer.forEach { outgoingEvents.append(.removeLocalItem($0)) }
    itemsToCreateInLocalContainer.downloaded.forEach { outgoingEvents.append(.createLocalItem($0)) }
    itemsToUpdateInLocalContainer.downloaded.forEach { outgoingEvents.append(.updateLocalItem($0)) }

    return outgoingEvents
  }

  private func resolveInitialSynchronizationConflicts(localContents: LocalContents, cloudContents: CloudContents) -> [OutgoingEvent] {
    let itemsInInitialConflict = localContents.filter { cloudContents.containsByName($0) }
    guard !itemsInInitialConflict.isEmpty else {
      return []
    }
    return itemsInInitialConflict.map { .resolveInitialSynchronizationConflict($0) }
  }

  private static func getItemsToRemoveFromCloudContainer(currentLocalContents: LocalContents, newLocalContents: LocalContents) -> LocalContents {
    currentLocalContents.filter { !newLocalContents.containsByName($0) }
  }

  private static func getItemsToCreateInCloudContainer(notTrashedCloudContents: CloudContents, trashedCloudContents: CloudContents, localContents: LocalContents) -> LocalContents {
    localContents.reduce(into: LocalContents()) { result, localItem in
      if !notTrashedCloudContents.containsByName(localItem) && !trashedCloudContents.containsByName(localItem) {
        result.append(localItem)
      } else if !notTrashedCloudContents.containsByName(localItem),
                let trashedCloudItem = trashedCloudContents.firstByName(localItem),
                trashedCloudItem.lastModificationDate < localItem.lastModificationDate {
        // If Cloud .Trash contains item and it's last modification date is less than the local item's last modification date than file should be recreated.
        result.append(localItem)
      }
    }
  }

  private static func getItemsToUpdateInCloudContainer(notTrashedCloudContents: CloudContents, localContents: LocalContents, isInitialSynchronization: Bool) -> LocalContents {
    guard !isInitialSynchronization else { return [] }
    // Due to the initial sync all conflicted local items will be duplicated with different name and replaced by the cloud items to avoid a data loss.
    return localContents.reduce(into: LocalContents()) { result, localItem in
      if let cloudItem = notTrashedCloudContents.firstByName(localItem),
         localItem.lastModificationDate > cloudItem.lastModificationDate {
        result.append(localItem)
      }
    }
  }

  private static func getItemsWithErrors(_ cloudContents: CloudContents) -> [SynchronizationError] {
     cloudContents.reduce(into: [SynchronizationError](), { partialResult, cloudItem in
       if let downloadingError = cloudItem.downloadingError, let synchronizationError = downloadingError.ubiquitousError {
        partialResult.append(synchronizationError)
      }
       if let uploadingError = cloudItem.uploadingError, let synchronizationError = uploadingError.ubiquitousError {
        partialResult.append(synchronizationError)
      }
    })
  }

  private static func getItemsToRemoveFromLocalContainer(notTrashedCloudContents: CloudContents, trashedCloudContents: CloudContents, localContents: LocalContents) -> CloudContents {
    trashedCloudContents.reduce(into: CloudContents()) { result, cloudItem in
      // Items shouldn't be removed if newer version of the item isn't in the trash.
      if let notTrashedCloudItem = notTrashedCloudContents.firstByName(cloudItem), notTrashedCloudItem.lastModificationDate > cloudItem.lastModificationDate {
        return
      }
      if let localItemValue = localContents.firstByName(cloudItem),
         cloudItem.lastModificationDate >= localItemValue.lastModificationDate {
        result.append(cloudItem)
      }
    }
  }

  private static func getItemsToCreateInLocalContainer(notTrashedCloudContents: CloudContents, localContents: LocalContents) -> CloudContents {
    notTrashedCloudContents.withUnresolvedConflicts(false).filter { !localContents.containsByName($0) }
  }

  private static func getItemsToUpdateInLocalContainer(notTrashedCloudContents: CloudContents, localContents: LocalContents, isInitialSynchronization: Bool) -> CloudContents {
    notTrashedCloudContents.withUnresolvedConflicts(false).reduce(into: CloudContents()) { result, cloudItem in
      if let localItemValue = localContents.firstByName(cloudItem) {
        // Due to the initial sync all conflicted local items will be duplicated with different name and replaced by the cloud items to avoid a data loss.
        if isInitialSynchronization {
          result.append(cloudItem)
        } else if cloudItem.lastModificationDate > localItemValue.lastModificationDate {
          result.append(cloudItem)
        }
      }
    }
  }

  private static func getItemsToResolveConflicts(notTrashedCloudContents: CloudContents) -> CloudContents {
    notTrashedCloudContents.withUnresolvedConflicts(true)
  }
}
