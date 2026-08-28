protocol MetadataItem: Equatable, Hashable {
  var fileName: String { get }
  var fileUrl: URL { get }
  var lastModificationDate: TimeInterval { get }
}

struct LocalMetadataItem: MetadataItem {
  let fileName: String
  let fileUrl: URL
  let lastModificationDate: TimeInterval
}

struct CloudMetadataItem: MetadataItem {
  let fileName: String
  let fileUrl: URL
  var isDownloaded: Bool
  var percentDownloaded: NSNumber
  var lastModificationDate: TimeInterval
  let downloadingError: NSError?
  let uploadingError: NSError?
  let hasUnresolvedConflicts: Bool
}

extension LocalMetadataItem {
  init(fileUrl: URL) throws {
    let resources = try fileUrl.resourceValues(forKeys: [.contentModificationDateKey])
    guard let lastModificationDate = resources.contentModificationDate?.roundedTime else {
      LOG(.error, "Failed to initialize LocalMetadataItem from URL's resources: \(resources)")
      throw SynchronizationError.failedToCreateMetadataItem
    }
    fileName = fileUrl.lastPathComponent
    self.fileUrl = fileUrl.standardizedFileURL
    self.lastModificationDate = lastModificationDate
  }

  func fileData() throws -> Data {
    try Data(contentsOf: fileUrl)
  }
}

extension CloudMetadataItem {
  static let requiredAttributes = [
    NSMetadataItemFSNameKey,
    NSMetadataItemURLKey,
    NSMetadataUbiquitousItemDownloadingStatusKey,
    NSMetadataUbiquitousItemPercentDownloadedKey,
    NSMetadataItemFSContentChangeDateKey,
    NSMetadataUbiquitousItemHasUnresolvedConflictsKey,
  ]

  private static let metadataAttributes = requiredAttributes + [
    NSMetadataUbiquitousItemDownloadingErrorKey,
    NSMetadataUbiquitousItemUploadingErrorKey,
  ]

  /// Builds the item, or reports the attributes whose absence or type made it fail.
  static func make(from values: [String: Any]) -> (item: CloudMetadataItem?, invalid: [String]) {
    var invalid = [String]()
    func value<T>(_ key: String, as _: T.Type = T.self) -> T? {
      guard let rawValue = values[key] else {
        invalid.append("\(key) (missing)")
        return nil
      }
      guard let value = rawValue as? T else {
        invalid.append("\(key) (\(type(of: rawValue)))")
        return nil
      }
      return value
    }

    // Parse every attribute before checking the result so one record names every bad value.
    let fileName: String? = value(NSMetadataItemFSNameKey)
    let fileUrl: URL? = value(NSMetadataItemURLKey)
    let downloadStatus: String? = value(NSMetadataUbiquitousItemDownloadingStatusKey)
    let percentDownloaded: NSNumber? = value(NSMetadataUbiquitousItemPercentDownloadedKey)
    let modificationDate: Date? = value(NSMetadataItemFSContentChangeDateKey)
    let hasUnresolvedConflicts: Bool? = value(NSMetadataUbiquitousItemHasUnresolvedConflictsKey)
    guard let fileName,
          let fileUrl,
          let downloadStatus,
          let percentDownloaded,
          let modificationDate,
          let hasUnresolvedConflicts
    else {
      return (nil, invalid)
    }

    let item = CloudMetadataItem(fileName: fileName,
                                 fileUrl: fileUrl.standardizedFileURL,
                                 isDownloaded: downloadStatus == NSMetadataUbiquitousItemDownloadingStatusCurrent,
                                 percentDownloaded: percentDownloaded,
                                 lastModificationDate: modificationDate.roundedTime,
                                 downloadingError: values[NSMetadataUbiquitousItemDownloadingErrorKey] as? NSError,
                                 uploadingError: values[NSMetadataUbiquitousItemUploadingErrorKey] as? NSError,
                                 hasUnresolvedConflicts: hasUnresolvedConflicts)
    return (item, invalid)
  }

  init(metadataItem: NSMetadataItem) throws {
    let values = metadataItem.values(forAttributes: Self.metadataAttributes) ?? [:]
    let (item, invalid) = Self.make(from: values)
    guard let item else {
      // The joined names stay well inside the per-record limit of the system log.
      assert(!invalid.isEmpty)
      LOG(.error, "Failed to initialize CloudMetadataItem. Invalid attributes: \(invalid.joined(separator: ", "))")
      let allValues = metadataItem.values(forAttributes: metadataItem.attributes) ?? [:]
      for (attribute, value) in allValues {
        LOG(.debug, "\(attribute): \(value)")
      }
      throw SynchronizationError.failedToCreateMetadataItem
    }
    self = item
  }

  func relatedLocalItemUrl(to localContainer: URL) -> URL {
    localContainer.appendingPathComponent(fileName)
  }
}

extension MetadataItem {
  var shortDebugDescription: String {
    "fileName: \(fileName), lastModified: \(lastModificationDate)"
  }
}

extension CloudMetadataItem {
  var synchronizationDebugDescription: String {
    "lastModified: \(lastModificationDate), url: \(fileUrl.path), downloaded: \(isDownloaded), percentDownloaded: \(percentDownloaded), unresolvedConflicts: \(hasUnresolvedConflicts), downloadingError: \(Self.errorCode(downloadingError)), uploadingError: \(Self.errorCode(uploadingError))"
  }

  private static func errorCode(_ error: NSError?) -> String {
    error.map { "\($0.domain)#\($0.code)" } ?? "none"
  }
}

extension LocalMetadataItem {
  func relatedCloudItemUrl(to cloudContainer: URL) -> URL {
    cloudContainer.appendingPathComponent(fileName)
  }
}

extension Array where Element: MetadataItem {
  func containsByName(_ item: any MetadataItem) -> Bool {
    contains(where: { $0.fileName == item.fileName })
  }

  func firstByName(_ item: any MetadataItem) -> Element? {
    first(where: { $0.fileName == item.fileName })
  }
}

extension Array where Element == CloudMetadataItem {
  var downloaded: Self {
    filter(\.isDownloaded)
  }

  var notDownloaded: Self {
    filter { !$0.isDownloaded && $0.percentDownloaded == 0.0 }
  }

  func withUnresolvedConflicts(_ hasUnresolvedConflicts: Bool) -> Self {
    filter { $0.hasUnresolvedConflicts == hasUnresolvedConflicts }
  }
}

private extension Date {
  var roundedTime: TimeInterval {
    timeIntervalSince1970.rounded(.down)
  }
}
