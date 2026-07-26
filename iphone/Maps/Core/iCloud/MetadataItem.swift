protocol MetadataItem: Equatable, Hashable {
  var fileName: String { get }
  var fileUrl: URL { get }
  var lastModificationDate: TimeInterval { get }
  var size: Int64 { get }
}

extension MetadataItem {
  /// Cheap identity of the file's content. It is used to skip recomputing the fingerprint of an unchanged file
  /// and never to decide which version of the file wins.
  var identity: FileIdentity { FileIdentity(modificationDate: lastModificationDate, size: size) }

  var shortDebugDescription: String { "\(fileName) (modified: \(lastModificationDate), size: \(size))" }
}

struct LocalMetadataItem: MetadataItem {
  let fileName: String
  let fileUrl: URL
  let lastModificationDate: TimeInterval
  let size: Int64
}

struct CloudMetadataItem: MetadataItem {
  let fileName: String
  let fileUrl: URL
  let lastModificationDate: TimeInterval
  let size: Int64
  let isDownloaded: Bool
  let hasUnresolvedConflicts: Bool
  let downloadingError: NSError?
  let uploadingError: NSError?
}

extension LocalMetadataItem {
  init?(fileUrl: URL) {
    guard let resources = try? fileUrl.resourceValues(forKeys: [.contentModificationDateKey, .fileSizeKey]),
          let lastModificationDate = resources.contentModificationDate?.timeIntervalSince1970
    else {
      LOG(.warning, "Failed to read attributes of the local file \(fileUrl.lastPathComponent)")
      return nil
    }
    fileName = fileUrl.lastPathComponent
    self.fileUrl = fileUrl.standardizedFileURL
    self.lastModificationDate = lastModificationDate
    size = Int64(resources.fileSize ?? 0)
  }

  func relatedCloudItemUrl(to cloudContainer: URL) -> URL {
    cloudContainer.appendingPathComponent(fileName)
  }
}

extension CloudMetadataItem {
  /// A single result of the metadata query. iCloud regularly reports items with incomplete attributes: such an
  /// item still proves that the file exists, but it cannot be used for file operations.
  enum Observation {
    /// The item is complete enough to be used for file operations.
    case actionable(CloudMetadataItem)
    /// The file is known to exist in iCloud but is not usable right now.
    case unusable(fileName: String, missingAttributes: [String])
    /// The item cannot even be identified: the snapshot it belongs to is incomplete.
    case unidentifiable(missingAttributes: [String])
  }

  static func observation(from metadataItem: NSMetadataItem) -> Observation {
    var missingAttributes = [String]()
    func required<T>(_ key: String, as _: T.Type) -> T? {
      guard let value = metadataItem.value(forAttribute: key) as? T else {
        missingAttributes.append(key)
        return nil
      }
      return value
    }

    let fileName = required(NSMetadataItemFSNameKey, as: String.self)
    let fileUrl = required(NSMetadataItemURLKey, as: URL.self)
    let lastModificationDate = required(NSMetadataItemFSContentChangeDateKey, as: Date.self)?.timeIntervalSince1970
    let downloadingStatus = required(NSMetadataUbiquitousItemDownloadingStatusKey, as: String.self)

    guard let fileName = fileName ?? fileUrl?.lastPathComponent else {
      return .unidentifiable(missingAttributes: missingAttributes)
    }
    guard let fileUrl, let lastModificationDate, let downloadingStatus else {
      return .unusable(fileName: fileName, missingAttributes: missingAttributes)
    }
    // Attributes below are not required to perform file operations and have safe defaults.
    let size = metadataItem.value(forAttribute: NSMetadataItemFSSizeKey) as? NSNumber
    let hasUnresolvedConflicts = metadataItem.value(forAttribute: NSMetadataUbiquitousItemHasUnresolvedConflictsKey) as? Bool
    return .actionable(CloudMetadataItem(fileName: fileName,
                                         fileUrl: fileUrl.standardizedFileURL,
                                         lastModificationDate: lastModificationDate,
                                         size: size?.int64Value ?? 0,
                                         isDownloaded: downloadingStatus == NSMetadataUbiquitousItemDownloadingStatusCurrent,
                                         hasUnresolvedConflicts: hasUnresolvedConflicts ?? false,
                                         downloadingError: metadataItem.value(forAttribute: NSMetadataUbiquitousItemDownloadingErrorKey) as? NSError,
                                         uploadingError: metadataItem.value(forAttribute: NSMetadataUbiquitousItemUploadingErrorKey) as? NSError))
  }

  var synchronizationError: SynchronizationError? {
    downloadingError?.ubiquitousError ?? uploadingError?.ubiquitousError
  }

  func relatedLocalItemUrl(to localContainer: URL) -> URL {
    localContainer.appendingPathComponent(fileName)
  }
}

// MARK: - DirectorySnapshot

/// The whole content of a synchronized directory as it was observed at some moment.
/// Only a complete snapshot may be used to conclude that a file is gone.
struct DirectorySnapshot<Item: MetadataItem>: Equatable {
  enum ItemState {
    /// The file is present and can be used for file operations.
    case present(Item)
    /// The file is present but cannot be used right now.
    case unavailable
    /// The file is not in the directory and the snapshot is trustworthy.
    case absent
    /// The directory was not observed in full: nothing can be concluded about the file.
    case unknown

    var item: Item? {
      if case .present(let item) = self {
        return item
      }
      return nil
    }
  }

  /// Files that exist but whose attributes or content are not available: they prove presence only.
  let unavailableFileNames: Set<String>
  /// False when some of the directory content could not be observed: absence proves nothing.
  let isComplete: Bool

  private let itemsByName: [String: Item]

  init(items: [Item], unavailableFileNames: Set<String> = [], isComplete: Bool = true) {
    self.unavailableFileNames = unavailableFileNames
    self.isComplete = isComplete
    // The same name may be reported more than once: keep the most recently modified file.
    itemsByName = Dictionary(items.map { ($0.fileName, $0) },
                             uniquingKeysWith: { $0.lastModificationDate >= $1.lastModificationDate ? $0 : $1 })
  }

  /// Files that can be used for file operations.
  var items: [Item] { itemsByName.values.sorted { $0.fileName < $1.fileName } }

  var fileNames: Set<String> { Set(itemsByName.keys).union(unavailableFileNames) }

  func state(of fileName: String) -> ItemState {
    if let item = itemsByName[fileName] {
      return .present(item)
    }
    if unavailableFileNames.contains(fileName) {
      return .unavailable
    }
    return isComplete ? .absent : .unknown
  }

  var shortDebugDescription: String {
    var description = "\(items.count) file(s)\(isComplete ? "" : ", incomplete")"
    if !unavailableFileNames.isEmpty {
      description += ", unavailable: \(unavailableFileNames.sorted().joined(separator: ", "))"
    }
    return items.reduce(into: description) { $0 += "\n\($1.shortDebugDescription)" }
  }
}
