import XCTest
@testable import Organic_Maps__Debug_

final class SynchronizationStateManagerTests: XCTestCase {

  var syncStateManager: SynchronizationStateManager!
  var outgoingEvents: [OutgoingEvent] = []

  override func setUp() {
    super.setUp()
    syncStateManager = DefaultSynchronizationStateManager(isInitialSynchronization: false)
  }

  override func tearDown() {
    syncStateManager = nil
    outgoingEvents.removeAll()
    super.tearDown()
  }
  // MARK: - Test didFinishGathering without errors and on initial synchronization
  func testInitialSynchronization() {
    syncStateManager = DefaultSynchronizationStateManager(isInitialSynchronization: true)

    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1))
    let localItem2 = LocalMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(3)) // Local only item

    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(2), isInTrash: false) // Conflicting item
    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(4), isInTrash: false) // Cloud only item

    let localItems: LocalContents = [localItem1, localItem2]
    let cloudItems: CloudContents = [cloudItem1, cloudItem3]

    outgoingEvents.append(contentsOf: syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems)))
    outgoingEvents.append(contentsOf: syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems)))

    XCTAssertTrue(outgoingEvents.contains { event in
      if case .resolveInitialSynchronizationConflict(let item) = event, item == localItem1 {
        return true
      }
      return false
    }, "Expected to resolve initial synchronization conflict for localItem1")

    XCTAssertTrue(outgoingEvents.contains { event in
      if case .createLocalItem(let item) = event, item == cloudItem3 {
        return true
      }
      return false
    }, "Expected to create local item for cloudItem3")

    XCTAssertTrue(outgoingEvents.contains { event in
      if case .createCloudItem(let item) = event, item == localItem2 {
        return true
      }
      return false
    }, "Expected to create cloud item for localItem2")

    XCTAssertTrue(outgoingEvents.contains { event in
      if case .didFinishInitialSynchronization = event {
        return true
      }
      return false
    }, "Expected to finish initial synchronization")
  }

  func testInitialSynchronizationWithNewerCloudItem() {
    syncStateManager = DefaultSynchronizationStateManager(isInitialSynchronization: true)

    let localItem = LocalMetadataItem.stub(fileName: "file", lastModificationDate: TimeInterval(1))
    let cloudItem = CloudMetadataItem.stub(fileName: "file", lastModificationDate: TimeInterval(2), isInTrash: false)

    outgoingEvents.append(contentsOf: syncStateManager.resolveEvent(.didFinishGatheringLocalContents([localItem])))
    outgoingEvents.append(contentsOf: syncStateManager.resolveEvent(.didFinishGatheringCloudContents([cloudItem])))

    XCTAssertTrue(outgoingEvents.contains { if case .resolveInitialSynchronizationConflict(_) = $0 { return true } else { return false } }, "Expected conflict resolution for a newer cloud item")
  }

  func testInitialSynchronizationWithNewerLocalItem() {
    syncStateManager = DefaultSynchronizationStateManager(isInitialSynchronization: true)

    let localItem = LocalMetadataItem.stub(fileName: "file", lastModificationDate: TimeInterval(2))
    let cloudItem = CloudMetadataItem.stub(fileName: "file", lastModificationDate: TimeInterval(1), isInTrash: false)

    outgoingEvents.append(contentsOf: syncStateManager.resolveEvent(.didFinishGatheringLocalContents([localItem])))
    outgoingEvents.append(contentsOf: syncStateManager.resolveEvent(.didFinishGatheringCloudContents([cloudItem])))

    XCTAssertTrue(outgoingEvents.contains { if case .resolveInitialSynchronizationConflict(_) = $0 { return true } else { return false } }, "Expected conflict resolution for a newer local item")
  }

  func testInitialSynchronizationWithNonConflictingItems() {
    syncStateManager = DefaultSynchronizationStateManager(isInitialSynchronization: true)

    let localItem = LocalMetadataItem.stub(fileName: "localFile", lastModificationDate: TimeInterval(1))
    let cloudItem = CloudMetadataItem.stub(fileName: "cloudFile", lastModificationDate: TimeInterval(2), isInTrash: false)

    outgoingEvents.append(contentsOf: syncStateManager.resolveEvent(.didFinishGatheringLocalContents([localItem])))
    outgoingEvents.append(contentsOf: syncStateManager.resolveEvent(.didFinishGatheringCloudContents([cloudItem])))

    XCTAssertTrue(outgoingEvents.contains { if case .createLocalItem(_) = $0 { return true } else { return false } }, "Expected creation of local item for cloudFile")
    XCTAssertTrue(outgoingEvents.contains { if case .createCloudItem(_) = $0 { return true } else { return false } }, "Expected creation of cloud item for localFile")
  }

  // MARK: - Test didFinishGathering without errors and after initial synchronization
  func testDidFinishGatheringWhenCloudAndLocalIsEmpty() {
    let localItems: LocalContents = []
    let cloudItems: CloudContents = []

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))
    XCTAssertEqual(outgoingEvents.count, 0)

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    XCTAssertEqual(outgoingEvents.count, 0)
  }

  func testDidFinishGatheringWhenOnlyCloudIsEmpty() {
    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1))
    let localItem2 = LocalMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2))
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3))

    let localItems: LocalContents = LocalContents([localItem1, localItem2, localItem3])
    let cloudItems: CloudContents = []

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))

    XCTAssertEqual(outgoingEvents.count, 0)

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    outgoingEvents.forEach { event in
      switch event {
      case .createCloudItem(let item):
        XCTAssertTrue(localItems.containsByName(item))
      default:
        XCTFail()
      }
    }
  }

  func testDidFinishGatheringWhenOnlyLocalIsEmpty() {
    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1), isInTrash: false)
    let cloudItem2 = CloudMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2), isInTrash: false)
    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3), isInTrash: false)

    let localItems = LocalContents()
    let cloudItems = CloudContents([cloudItem1, cloudItem2, cloudItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))
    XCTAssertEqual(outgoingEvents.count, 0)

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    outgoingEvents.forEach { event in
      switch event {
      case .createLocalItem(let item):
        XCTAssertTrue(cloudItems.containsByName(item))
      default:
        XCTFail()
      }
    }
  }

  func testDidFinishGatheringWhenLocalIsEmptyAndAllCloudFilesWasDeleted() {
    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1))
    let localItem2 = LocalMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2))
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3))

    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(2), isInTrash: true)
    let cloudItem2 = CloudMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(3), isInTrash: true)
    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(4), isInTrash: true)

    let localItems = [localItem1, localItem2, localItem3]
    let cloudItems = [cloudItem1, cloudItem2, cloudItem3]

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))
    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))

    XCTAssertEqual(outgoingEvents.count, 3)
    outgoingEvents.forEach { event in
      switch event {
      case .removeLocalItem(let item):
        XCTAssertTrue(localItems.containsByName(item))
      default:
        XCTFail()
      }
    }
  }

  func testDidFinishGatheringWhenLocalIsNotEmptyAndAllCloudFilesWasDeleted() {
    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1), isInTrash: true)
    let cloudItem2 = CloudMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2), isInTrash: true)
    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3), isInTrash: true)

    let localItems = LocalContents()
    let cloudItems = CloudContents([cloudItem1, cloudItem2, cloudItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))
    XCTAssertEqual(outgoingEvents.count, 0)

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    XCTAssertEqual(outgoingEvents.count, 0)
  }

  func testDidFinishGatheringWhenLocalIsEmptyAndSomeCloudFilesWasDeleted() {
    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1), isInTrash: true)
    let cloudItem2 = CloudMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2), isInTrash: true)
    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3), isInTrash: false)

    let localItems = LocalContents()
    let cloudItems = CloudContents([cloudItem1, cloudItem2, cloudItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))
    XCTAssertEqual(outgoingEvents.count, 0)

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    XCTAssertEqual(outgoingEvents.count, 1)
    outgoingEvents.forEach { event in
      switch event {
      case .createLocalItem(let item):
        XCTAssertEqual(item, cloudItem3)
      default:
        XCTFail()
      }
    }
  }

  func testDidFinishGatheringWhenLocalAndCloudAreNotEmptyAndEqual() {
    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1))
    let localItem2 = LocalMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2))
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3))

    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1), isInTrash: false)
    let cloudItem2 = CloudMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2), isInTrash: false)
    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3), isInTrash: false)

    let localItems = LocalContents([localItem1, localItem2, localItem3])
    let cloudItems = CloudContents([cloudItem1, cloudItem2, cloudItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    XCTAssertEqual(outgoingEvents.count, 0)

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))
    XCTAssertEqual(outgoingEvents.count, 0)
  }

  func testDidFinishGatheringWhenLocalAndCloudAreNotEmptyAndSomeLocalItemsAreNewer() {
    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1))
    let localItem2 = LocalMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(3))
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(4))

    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1), isInTrash: false)
    let cloudItem2 = CloudMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2), isInTrash: false)
    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3), isInTrash: false)

    let localItems = LocalContents([localItem1, localItem2, localItem3])
    let cloudItems = CloudContents([cloudItem1, cloudItem2, cloudItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    XCTAssertEqual(outgoingEvents.count, 0)

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))
    XCTAssertEqual(outgoingEvents.count, 2)
    outgoingEvents.forEach { event in
      switch event {
      case .updateCloudItem(let item):
        XCTAssertTrue([localItem2, localItem3].containsByName(item))
      default:
        XCTFail()
      }
    }
  }

  func testDidFinishGatheringWhenLocalAndCloudAreNotEmptyAndSomeCloudItemsAreNewer() {
    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1))
    let localItem2 = LocalMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2))
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3))

    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(4), isInTrash: false)
    let cloudItem2 = CloudMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2), isInTrash: false)
    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(7), isInTrash: false)

    let localItems = LocalContents([localItem1, localItem2, localItem3])
    let cloudItems = CloudContents([cloudItem1, cloudItem2, cloudItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    XCTAssertEqual(outgoingEvents.count, 0)

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))
    XCTAssertEqual(outgoingEvents.count, 2)
    outgoingEvents.forEach { event in
      switch event {
      case .updateLocalItem(let item):
        XCTAssertTrue([cloudItem1, cloudItem3].containsByName(item))
      default:
        XCTFail()
      }
    }
  }

  func testDidFinishGatheringWhenLocalAndCloudAreNotEmptyMixed() {
    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1))
    let localItem2 = LocalMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(3))
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3))
    let localItem4 = LocalMetadataItem.stub(fileName: "file4", lastModificationDate: TimeInterval(1))

    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(4), isInTrash: false)
    let cloudItem2 = CloudMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2), isInTrash: false)
    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(7), isInTrash: true)

    let localItems = LocalContents([localItem1, localItem2, localItem3, localItem4])
    let cloudItems = CloudContents([cloudItem1, cloudItem2, cloudItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    XCTAssertEqual(outgoingEvents.count, 0)

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))
    XCTAssertEqual(outgoingEvents.count, 4)
    outgoingEvents.forEach { event in
      switch event {
      case .updateLocalItem(let item):
        XCTAssertEqual(item, cloudItem1)
      case .removeLocalItem(let item):
        XCTAssertEqual(item, cloudItem3)
      case .createCloudItem(let item):
        XCTAssertEqual(item, localItem4)
      case .updateCloudItem(let item):
        XCTAssertEqual(item, localItem2)
      default:
        XCTFail()
      }
    }
  }

  func testDidFinishGatheringWhenCloudHaveTrashedNewerThanLocal() {
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3))

    let cloudItem3Trashed = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(8), isInTrash: true)

    let localItems = LocalContents([localItem3])
    let cloudItems = CloudContents([cloudItem3Trashed])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    XCTAssertEqual(outgoingEvents.count, 0)

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))
    XCTAssertEqual(outgoingEvents.count, 1)
    outgoingEvents.forEach { event in
      switch event {
      case .removeLocalItem(let item):
        XCTAssertEqual(item, cloudItem3Trashed)
      default:
        XCTFail()
      }
    }
  }

  func testDidFinishGatheringWhenLocallIsEmptyAndCloudHaveSameFileBothInTrashedAndNotAndTrashedOlder() {
    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(7), isInTrash: false)
    let cloudItem3Trashed = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(6), isInTrash: true)

    let localItems = LocalContents([])
    let cloudItems = CloudContents([cloudItem3, cloudItem3Trashed])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    XCTAssertEqual(outgoingEvents.count, 0)

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))
    XCTAssertEqual(outgoingEvents.count, 1)
    outgoingEvents.forEach { event in
      switch event {
      case .createLocalItem(let item):
        XCTAssertEqual(item, cloudItem3)
      default:
        XCTFail()
      }
    }
  }

  func testDidFinishGatheringWhenCloudHaveSameFileBothInTrashedAndNotAndTrashedOlder() {
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3))

    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(7), isInTrash: false)
    let cloudItem3Trashed = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(6), isInTrash: true)

    let localItems = LocalContents([localItem3])
    let cloudItems = CloudContents([cloudItem3, cloudItem3Trashed])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    XCTAssertEqual(outgoingEvents.count, 0)

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))
    XCTAssertEqual(outgoingEvents.count, 1)
    outgoingEvents.forEach { event in
      switch event {
      case .updateLocalItem(let item):
        XCTAssertEqual(item, cloudItem3)
      default:
        XCTFail()
      }
    }
  }

  func testDidFinishGatheringWhenCloudHaveSameFileBothInTrashedAndNotAndTrashedBotLocalIsNewer() {
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(9))

    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(2), isInTrash: false)
    let cloudItem3Trashed = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(6), isInTrash: true)

    let localItems = LocalContents([localItem3])
    let cloudItems = CloudContents([cloudItem3, cloudItem3Trashed])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    XCTAssertEqual(outgoingEvents.count, 0)

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))
    XCTAssertEqual(outgoingEvents.count, 1)
    outgoingEvents.forEach { event in
      switch event {
      case .updateCloudItem(let item):
        XCTAssertEqual(item, localItem3)
      default:
        XCTFail()
      }
    }
  }

  func testDidFinishGatheringWhenUpdatetLocallyItemSameAsDeletedFromCloudOnTheOtherDevice() {
    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1))

    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1), isInTrash: true)

    let localItems = LocalContents([localItem1])
    let cloudItems = CloudContents([cloudItem1])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))

    var localItemsToRemove: LocalContents = []
    XCTAssertEqual(outgoingEvents.count, 1)
    outgoingEvents.forEach { event in
      switch event {
      case .removeLocalItem(let cloudMetadataItem):
        XCTAssertEqual(cloudMetadataItem, cloudItem1)
        if let localItemToRemove = localItems.firstByName(cloudMetadataItem) {
          localItemsToRemove.append(localItemToRemove)
        }
      default:
        XCTFail()
      }
    }

    outgoingEvents = syncStateManager.resolveEvent(.didUpdateLocalContents(localItemsToRemove))
    XCTAssertEqual(outgoingEvents.count, 0)
  }

  // MARK: - Test didFinishGathering MergeConflicts
  func testDidFinishGatheringMergeConflictWhenUpdatetLocallyItemNewerThanDeletedFromCloudOnTheOtherDevice() {
    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(2))

    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1), isInTrash: true)

    let localItems = LocalContents([localItem1])
    let cloudItems = CloudContents([cloudItem1])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))

    // Here should be a merge conflict. New Cloud file should be created.
    XCTAssertEqual(outgoingEvents.count, 1)
    outgoingEvents.forEach { event in
      switch event {
      case .createCloudItem(let cloudMetadataItem):
        XCTAssertEqual(cloudMetadataItem, localItem1)
      default:
        XCTFail()
      }
    }
  }


  // MARK: - Test didUpdateLocalContents
  func testDidUpdateLocalContentsWhenContentWasNotChanged() {
    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1))
    let localItem2 = LocalMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2))
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3))

    let localItems = LocalContents([localItem1, localItem2, localItem3])
    let cloudItems = CloudContents([])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))

    XCTAssertEqual(outgoingEvents.count, 3)

    let newLocalItems = LocalContents([localItem1, localItem2, localItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didUpdateLocalContents(newLocalItems))
    XCTAssertEqual(outgoingEvents.count, 3) // Should be equal to the previous results because cloudContent wasn't changed
  }

  func testDidUpdateLocalContentsWhenNewLocalItemWasAdded() {
    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1))
    let localItem2 = LocalMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2))
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3))

    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1), isInTrash: false)
    let cloudItem2 = CloudMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2), isInTrash: false)
    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3), isInTrash: false)

    let localItems = LocalContents([localItem1, localItem2, localItem3])
    let cloudItems = CloudContents([cloudItem1, cloudItem2, cloudItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))

    XCTAssertEqual(outgoingEvents.count, 0)

    let localItem4 = LocalMetadataItem.stub(fileName: "file4", lastModificationDate: TimeInterval(4))
    let newLocalItems = LocalContents([localItem1, localItem2, localItem3, localItem4])

    outgoingEvents = syncStateManager.resolveEvent(.didUpdateLocalContents(newLocalItems))
    XCTAssertEqual(outgoingEvents.count, 1)

    outgoingEvents.forEach { event in
      switch event {
      case .createCloudItem(let item):
        XCTAssertEqual(item, localItem4)
      default:
        XCTFail()
      }
    }
  }

  func testDidUpdateLocalContentsWhenLocalItemWasUpdated() {
    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1))
    let localItem2 = LocalMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2))
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3))

    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1), isInTrash: false)
    let cloudItem2 = CloudMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2), isInTrash: false)
    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3), isInTrash: false)

    let localItems = LocalContents([localItem1, localItem2, localItem3])
    let cloudItems = CloudContents([cloudItem1, cloudItem2, cloudItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))

    XCTAssertEqual(outgoingEvents.count, 0)

    let localItem2Updated = LocalMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(3))
    let localItem3Updated = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(4))

    let newLocalItems = LocalContents([localItem1, localItem2Updated, localItem3Updated])
    outgoingEvents = syncStateManager.resolveEvent(.didUpdateLocalContents(newLocalItems))
    XCTAssertEqual(outgoingEvents.count, 2)

    outgoingEvents.forEach { event in
      switch event {
      case .updateCloudItem(let item):
        XCTAssertTrue([localItem2Updated, localItem3Updated].containsByName(item))
      default:
        XCTFail()
      }
    }
  }

  func testDidUpdateLocalContentsWhenLocalItemWasRemoved() {
    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1))
    let localItem2 = LocalMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2))
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3))

    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1), isInTrash: false)
    let cloudItem2 = CloudMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2), isInTrash: false)
    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3), isInTrash: false)

    let localItems = LocalContents([localItem1, localItem2, localItem3])
    let cloudItems = CloudContents([cloudItem1, cloudItem2, cloudItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))

    XCTAssertEqual(outgoingEvents.count, 0)

    let newLocalItems = LocalContents([localItem1, localItem2])

    outgoingEvents = syncStateManager.resolveEvent(.didUpdateLocalContents(newLocalItems))
    XCTAssertEqual(outgoingEvents.count, 1)

    outgoingEvents.forEach { event in
      switch event {
      case .removeCloudItem(let item):
        XCTAssertEqual(item, localItem3)
      default:
        XCTFail()
      }
    }
  }

  func testDidUpdateLocalContentsComplexUpdate() {
    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1))
    let localItem2 = LocalMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2))
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3))

    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1), isInTrash: false)
    let cloudItem2 = CloudMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2), isInTrash: false)
    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3), isInTrash: false)

    let localItems = LocalContents([localItem1, localItem2, localItem3])
    let cloudItems = CloudContents([cloudItem1, cloudItem2, cloudItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))

    XCTAssertEqual(outgoingEvents.count, 0)

    let localItem1New = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(2))
    let localItem3New = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(4))
    let localItem4New = LocalMetadataItem.stub(fileName: "file4", lastModificationDate: TimeInterval(5))
    let localItem5New = LocalMetadataItem.stub(fileName: "file5", lastModificationDate: TimeInterval(5))

    let newLocalItems = LocalContents([localItem1New, localItem3New, localItem4New, localItem5New])

    outgoingEvents = syncStateManager.resolveEvent(.didUpdateLocalContents(newLocalItems))
    XCTAssertEqual(outgoingEvents.count, 5)

    outgoingEvents.forEach { event in
      switch event {
      case .createCloudItem(let localMetadataItem):
        XCTAssertTrue([localItem4New, localItem5New].containsByName(localMetadataItem))
      case .updateCloudItem(let localMetadataItem):
        XCTAssertTrue([localItem1New, localItem3New].containsByName(localMetadataItem))
      case .removeCloudItem(let localMetadataItem):
        XCTAssertEqual(localMetadataItem, localItem2)
      default:
        XCTFail()
      }
    }
  }

  // TODO: Test didUpdateCloudContents
  func testDidUpdateCloudContentsWhenContentWasNotChanged() {
    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1))
    let localItem2 = LocalMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2))
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3))

    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1), isInTrash: false)
    let cloudItem2 = CloudMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2), isInTrash: false)
    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3), isInTrash: false)

    let localItems = LocalContents([localItem1, localItem2, localItem3])
    let cloudItems = CloudContents([cloudItem1, cloudItem2, cloudItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))

    XCTAssertEqual(outgoingEvents.count, 0)

    let newCloudItems = CloudContents([cloudItem1, cloudItem2, cloudItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didUpdateCloudContents(newCloudItems))
    XCTAssertEqual(outgoingEvents.count, 0)
  }

  func testDidUpdateCloudContentsWhenContentItemWasAdded() {
    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1))
    let localItem2 = LocalMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2))
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3))

    let cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1), isInTrash: false)
    let cloudItem2 = CloudMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2), isInTrash: false)
    let cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3), isInTrash: false)

    let localItems = LocalContents([localItem1, localItem2, localItem3])
    var cloudItems = CloudContents([cloudItem1, cloudItem2, cloudItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))

    XCTAssertEqual(outgoingEvents.count, 0)

    var cloudItem4 = CloudMetadataItem.stub(fileName: "file4", lastModificationDate: TimeInterval(3), isInTrash: false, isDownloaded: false)
    cloudItems.append(cloudItem4)

    outgoingEvents = syncStateManager.resolveEvent(.didUpdateCloudContents(cloudItems))
    XCTAssertEqual(outgoingEvents.count, 1)
    outgoingEvents.forEach { event in
      switch event {
      case .startDownloading(let cloudMetadataItem):
        XCTAssertEqual(cloudMetadataItem, cloudItem4)
      default:
        XCTFail()
      }
    }

    cloudItem4.isDownloaded = true

    // recreate collection
    cloudItems = [cloudItem1, cloudItem2, cloudItem3, cloudItem4]
    outgoingEvents = syncStateManager.resolveEvent(.didUpdateCloudContents(cloudItems))

    XCTAssertEqual(outgoingEvents.count, 1)
    outgoingEvents.forEach { event in
      switch event {
      case .createLocalItem(let cloudMetadataItem):
        XCTAssertEqual(cloudMetadataItem, cloudItem4)
      default:
        XCTFail()
      }
    }
  }

  func testDidUpdateCloudContentsWhenAllContentWasTrashed() {
    let localItem1 = LocalMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1))
    let localItem2 = LocalMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2))
    let localItem3 = LocalMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3))

    var cloudItem1 = CloudMetadataItem.stub(fileName: "file1", lastModificationDate: TimeInterval(1), isInTrash: false)
    var cloudItem2 = CloudMetadataItem.stub(fileName: "file2", lastModificationDate: TimeInterval(2), isInTrash: false)
    var cloudItem3 = CloudMetadataItem.stub(fileName: "file3", lastModificationDate: TimeInterval(3), isInTrash: false)

    let localItems = LocalContents([localItem1, localItem2, localItem3])
    var cloudItems = CloudContents([cloudItem1, cloudItem2, cloudItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringLocalContents(localItems))
    outgoingEvents = syncStateManager.resolveEvent(.didFinishGatheringCloudContents(cloudItems))

    XCTAssertEqual(outgoingEvents.count, 0)

    cloudItem1.isRemoved = true
    cloudItem2.isRemoved = true
    cloudItem3.isRemoved = true

    cloudItems = CloudContents([cloudItem1, cloudItem2, cloudItem3])

    outgoingEvents = syncStateManager.resolveEvent(.didUpdateCloudContents(cloudItems))
    XCTAssertEqual(outgoingEvents.count, 3)

    var localItemsToRemove: LocalContents = []
    outgoingEvents.forEach { event in
      switch event {
      case .removeLocalItem(let cloudMetadataItem):
        XCTAssertTrue(cloudItems.containsByName(cloudMetadataItem))
        if let localItemToRemove = localItems.firstByName(cloudMetadataItem) {
          localItemsToRemove.append(localItemToRemove)
        }
      default:
        XCTFail()
      }
    }

    outgoingEvents = syncStateManager.resolveEvent(.didUpdateLocalContents(localItemsToRemove))
    // Because all cloud items in .trash and we have removed all local items, we should not have any outgoing events.
    XCTAssertEqual(outgoingEvents.count, 0)
  }
}

