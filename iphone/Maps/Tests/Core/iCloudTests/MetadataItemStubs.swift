@testable import Organic_Maps__Debug_

extension LocalMetadataItem {
  static func stub(fileName: String,
                   lastModificationDate: TimeInterval = 1,
                   size: Int64 = 0) -> LocalMetadataItem {
    LocalMetadataItem(fileName: fileName,
                      fileUrl: URL(fileURLWithPath: "/local/\(fileName)"),
                      lastModificationDate: lastModificationDate,
                      size: size)
  }
}

extension CloudMetadataItem {
  static func stub(fileName: String,
                   lastModificationDate: TimeInterval = 1,
                   size: Int64 = 0,
                   isDownloaded: Bool = true,
                   hasUnresolvedConflicts: Bool = false) -> CloudMetadataItem {
    CloudMetadataItem(fileName: fileName,
                      fileUrl: URL(fileURLWithPath: "/cloud/\(fileName)"),
                      lastModificationDate: lastModificationDate,
                      size: size,
                      isDownloaded: isDownloaded,
                      hasUnresolvedConflicts: hasUnresolvedConflicts,
                      downloadingError: nil,
                      uploadingError: nil)
  }
}

/// Metadata query result with the given attributes only. iCloud reports items with missing attributes.
final class MetadataItemMock: NSMetadataItem {
  private let values: [String: Any]

  init(_ values: [String: Any]) {
    self.values = values
    super.init()
  }

  override func value(forAttribute key: String) -> Any? { values[key] }
}

/// Metadata query that returns the given results only.
final class MetadataQueryMock: NSMetadataQuery {
  private let items: [NSMetadataItem]

  init(_ items: [NSMetadataItem]) {
    self.items = items
    super.init()
  }

  override var results: [Any] { items }
}

final class SynchronizationClockMock: SynchronizationClock {
  var activeTime: TimeInterval = 0

  func advance(by interval: TimeInterval) { activeTime += interval }
}

/// Returns the fingerprint of the content assigned to the item's URL. An item without content cannot be read.
final class FingerprintProviderMock: FingerprintProvider {
  var contents = [URL: String]()
  var onFingerprintReady: (() -> Void)?

  func fingerprint(of item: any MetadataItem) -> Fingerprint? {
    contents[item.fileUrl].map { Fingerprint(hashing: Data($0.utf8)) }
  }
}

final class SynchronizedStateStoreMock: SynchronizedStateStore {
  var states = [String: SynchronizedFileState]()

  func state(for fileName: String) -> SynchronizedFileState? { states[fileName] }

  func setState(_ state: SynchronizedFileState?, for fileName: String) { states[fileName] = state }

  func resetIfCloudIdentityChanged(_: Data) { states.removeAll() }
}
