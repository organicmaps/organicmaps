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
  private static let required: [(key: String, isValid: (Any) -> Bool)] = [
    (NSMetadataItemFSNameKey, { $0 is String }),
    (NSMetadataItemURLKey, { $0 is URL }),
    (NSMetadataUbiquitousItemDownloadingStatusKey, { $0 is String }),
    (NSMetadataUbiquitousItemPercentDownloadedKey, { $0 is NSNumber }),
    (NSMetadataItemFSContentChangeDateKey, { $0 is Date }),
    (NSMetadataUbiquitousItemHasUnresolvedConflictsKey, { $0 is Bool }),
  ]

  private static let requiredAttributes: [String] = required.map(\.key)

  /// Names of the required attributes that are missing or have an unexpected type.
  static func invalidAttributes(in values: [String: Any]) -> [String] {
    required.compactMap { key, isValid in
      guard let value = values[key] else { return "\(key) (missing)" }
      return isValid(value) ? nil : "\(key) (\(type(of: value)))"
    }
  }

  init(metadataItem: NSMetadataItem) throws {
    let values = metadataItem.values(forAttributes: Self.requiredAttributes) ?? [:]
    guard let fileName = values[NSMetadataItemFSNameKey] as? String,
          let fileUrl = values[NSMetadataItemURLKey] as? URL,
          let downloadStatus = values[NSMetadataUbiquitousItemDownloadingStatusKey] as? String,
          let percentDownloaded = values[NSMetadataUbiquitousItemPercentDownloadedKey] as? NSNumber,
          let lastModificationDate = (values[NSMetadataItemFSContentChangeDateKey] as? Date)?.roundedTime,
          let hasUnresolvedConflicts = values[NSMetadataUbiquitousItemHasUnresolvedConflictsKey] as? Bool
    else {
      // Keep each failure in its own system record so one long diagnostic cannot hide the other
      // invalid attribute names.
      for invalidAttribute in Self.invalidAttributes(in: values) {
        LOG(.error, "Failed to initialize CloudMetadataItem. Invalid attribute: \(invalidAttribute)")
      }
      for attribute in metadataItem.attributes {
        LOG(.debug, "\(attribute): \(String(describing: metadataItem.value(forAttribute: attribute)))")
      }
      throw SynchronizationError.failedToCreateMetadataItem
    }
    self.fileName = fileName
    self.fileUrl = fileUrl.standardizedFileURL
    isDownloaded = downloadStatus == NSMetadataUbiquitousItemDownloadingStatusCurrent
    self.percentDownloaded = percentDownloaded
    self.lastModificationDate = lastModificationDate
    self.hasUnresolvedConflicts = hasUnresolvedConflicts
    downloadingError = metadataItem.value(forAttribute: NSMetadataUbiquitousItemDownloadingErrorKey) as? NSError
    uploadingError = metadataItem.value(forAttribute: NSMetadataUbiquitousItemUploadingErrorKey) as? NSError
  }

  init(fileUrl: URL) throws {
    let resources = try fileUrl.resourceValues(forKeys: [.nameKey,
                                                         .contentModificationDateKey,
                                                         .ubiquitousItemDownloadingStatusKey,
                                                         .ubiquitousItemHasUnresolvedConflictsKey,
                                                         .ubiquitousItemDownloadingErrorKey,
                                                         .ubiquitousItemUploadingErrorKey])
    guard let downloadStatus = resources.ubiquitousItemDownloadingStatus,
          // Not used.
          // let percentDownloaded = resources.ubiquitousItemDownloadingStatus,
          let lastModificationDate = resources.contentModificationDate?.roundedTime,
          let hasUnresolvedConflicts = resources.ubiquitousItemHasUnresolvedConflicts
    else {
      LOG(.error, "Failed to initialize CloudMetadataItem from \(fileUrl) resources: \(resources.allValues)")
      throw SynchronizationError.failedToCreateMetadataItem
    }
    fileName = fileUrl.lastPathComponent
    self.fileUrl = fileUrl.standardizedFileURL
    let isDownloaded = downloadStatus.rawValue == NSMetadataUbiquitousItemDownloadingStatusCurrent
    self.isDownloaded = isDownloaded
    percentDownloaded = isDownloaded ? 0.0 : 100.0
    self.lastModificationDate = lastModificationDate
    self.hasUnresolvedConflicts = hasUnresolvedConflicts
    downloadingError = resources.ubiquitousItemDownloadingError
    uploadingError = resources.ubiquitousItemUploadingError
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
