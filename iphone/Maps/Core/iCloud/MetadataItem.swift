protocol MetadataItem: Equatable, Hashable {
  var fileName: String { get }
  var fileNameWithoutExtension: String { get }
  var fileUrl: URL { get }
  var fileSize: Int { get }
  var contentType: String { get }
  var creationDate: TimeInterval { get }
  var lastModificationDate: TimeInterval { get }
  var isDeleted: Bool { get }
}

struct LocalMetadataItem: MetadataItem {
  let fileName: String
  let fileUrl: URL
  let fileSize: Int
  let contentType: String
  let creationDate: TimeInterval
  let lastModificationDate: TimeInterval
  let isDeleted: Bool
}

struct CloudMetadataItem: MetadataItem {
  let fileName: String
  let fileUrl: URL
  let fileSize: Int
  let contentType: String
  let isDownloaded: Bool
  let creationDate: TimeInterval
  let lastModificationDate: TimeInterval
  let isDeleted: Bool
  let isRemoved: Bool
  let downloadingError: NSError?
  let uploadingError: NSError?
  let hasUnresolvedConflicts: Bool
}

extension LocalMetadataItem {
  init(fileUrl: URL) throws {
    let resources = try fileUrl.resourceValues(forKeys: [.fileSizeKey, .typeIdentifierKey, .contentModificationDateKey, .creationDateKey])
    guard let fileSize = resources.fileSize,
          let contentType = resources.typeIdentifier,
          let creationDate = resources.creationDate?.timeIntervalSince1970.rounded(.down),
          let lastModificationDate = resources.contentModificationDate?.timeIntervalSince1970.rounded(.down) else {
      throw NSError(domain: "LocalMetadataItem", code: 0, userInfo: [NSLocalizedDescriptionKey: "Failed to initialize LocalMetadataItem from URL"])
    }
    self.fileName = fileUrl.lastPathComponent
    self.fileUrl = fileUrl
    self.fileSize = fileSize
    self.contentType = contentType
    self.creationDate = creationDate
    self.lastModificationDate = lastModificationDate
    self.isDeleted = fileUrl.isDeleted
  }

  func fileData() throws -> Data {
    try Data(contentsOf: fileUrl)
  }
}

extension CloudMetadataItem {
  init(metadataItem: NSMetadataItem, isRemoved: Bool = false) throws {
    guard let fileName = metadataItem.value(forAttribute: NSMetadataItemFSNameKey) as? String,
          let fileUrl = metadataItem.value(forAttribute: NSMetadataItemURLKey) as? URL,
          let fileSize = metadataItem.value(forAttribute: NSMetadataItemFSSizeKey) as? Int,
          let contentType = metadataItem.value(forAttribute: NSMetadataItemContentTypeKey) as? String,
          let downloadStatus = metadataItem.value(forAttribute: NSMetadataUbiquitousItemDownloadingStatusKey) as? String,
          let creationDate = (metadataItem.value(forAttribute: NSMetadataItemFSCreationDateKey) as? Date)?.timeIntervalSince1970.rounded(.down),
          let lastModificationDate = (metadataItem.value(forAttribute: NSMetadataItemFSContentChangeDateKey) as? Date)?.timeIntervalSince1970.rounded(.down),
          let hasUnresolvedConflicts = metadataItem.value(forAttribute: NSMetadataUbiquitousItemHasUnresolvedConflictsKey) as? Bool else {
      let metadata = metadataItem.values(forAttributes: metadataItem.attributes)
      throw NSError(domain: "CloudMetadataItem", code: 0, userInfo: [NSLocalizedDescriptionKey: "Failed to initialize CloudMetadataItem from NSMetadataItem", NSDebugDescriptionErrorKey: metadata!.description])
    }
    self.fileName = fileName
    self.fileUrl = fileUrl
    self.fileSize = fileSize
    self.contentType = contentType
    self.isDownloaded = downloadStatus == NSMetadataUbiquitousItemDownloadingStatusCurrent
    self.creationDate = creationDate
    self.lastModificationDate = lastModificationDate
    self.isDeleted = fileUrl.isDeleted
    self.isRemoved = isRemoved || fileUrl.isTrashed
    self.hasUnresolvedConflicts = hasUnresolvedConflicts
    self.downloadingError = metadataItem.value(forAttribute: NSMetadataUbiquitousItemDownloadingErrorKey) as? NSError
    self.uploadingError = metadataItem.value(forAttribute: NSMetadataUbiquitousItemUploadingErrorKey) as? NSError
  }

  init(fileUrl: URL, isRemoved: Bool = false) throws {
    let resources = try fileUrl.resourceValues(forKeys: [.nameKey, .fileSizeKey, .typeIdentifierKey, .contentModificationDateKey, .creationDateKey, .ubiquitousItemDownloadingStatusKey, .ubiquitousItemHasUnresolvedConflictsKey, .ubiquitousItemDownloadingErrorKey, .ubiquitousItemUploadingErrorKey])
    guard let fileSize = resources.fileSize,
          let contentType = resources.typeIdentifier,
          let creationDate = resources.creationDate?.timeIntervalSince1970.rounded(.down),
          let downloadStatus = resources.ubiquitousItemDownloadingStatus,
          let lastModificationDate = resources.contentModificationDate?.timeIntervalSince1970.rounded(.down),
          let hasUnresolvedConflicts = resources.ubiquitousItemHasUnresolvedConflicts else {
      throw NSError(domain: "CloudMetadataItem", code: 0, userInfo: [NSLocalizedDescriptionKey: "Failed to initialize CloudMetadataItem from fileURL"])
    }
    self.fileName = fileUrl.lastPathComponent
    self.fileUrl = fileUrl
    self.fileSize = fileSize
    self.contentType = contentType
    self.isDownloaded = downloadStatus.rawValue == NSMetadataUbiquitousItemDownloadingStatusCurrent
    self.creationDate = creationDate
    self.lastModificationDate = lastModificationDate
    self.isDeleted = fileUrl.isDeleted
    self.isRemoved = isRemoved || fileUrl.isTrashed
    self.hasUnresolvedConflicts = hasUnresolvedConflicts
    self.downloadingError = resources.ubiquitousItemDownloadingError
    self.uploadingError = resources.ubiquitousItemUploadingError
  }

  func relatedLocalItemUrl(to localContainer: URL) -> URL {
    localContainer.appendingPathComponent(fileName)
  }
}

extension LocalMetadataItem {
  func relatedCloudItemUrl(to cloudContainer: URL) -> URL {
    cloudContainer.appendingPathComponent(fileName)
  }
}

extension Array where Element: MetadataItem {
  func containsByName(_ item: any MetadataItem) -> Bool {
    return contains(where: { $0.fileName == item.fileName })
  }

  func containsByNameWithoutExtension(_ item: any MetadataItem) -> Bool {
    return contains(where: { $0.fileNameWithoutExtension == item.fileNameWithoutExtension })
  }

  func firstByName(_ item: any MetadataItem) -> Element? {
    return first(where: { $0.fileName == item.fileName })
  }

  func firstByNameWithoutExtension(_ item: any MetadataItem) -> Element? {
    return first(where: { $0.fileNameWithoutExtension == item.fileNameWithoutExtension })
  }
}

extension Array where Element == CloudMetadataItem {
  var downloaded: Self {
    filter { $0.isDownloaded }
  }

  var notDownloaded: Self {
    filter { !$0.isDownloaded }
  }

  func withUnresolvedConflicts(_ hasUnresolvedConflicts: Bool) -> Self {
    filter { $0.hasUnresolvedConflicts == hasUnresolvedConflicts }
  }
}

extension MetadataItem {
  var fileNameWithoutExtension: String {
    // The file name may contain the deleted file extension, so we need to remove it first.
    isDeleted ? fileUrl.deletingPathExtension().deletingPathExtension().lastPathComponent : fileUrl.deletingPathExtension().lastPathComponent
  }
}

private extension URL {
  var isDeleted: Bool {
    pathExtension == FileType.deleted.fileExtension
  }

  var isTrashed: Bool {
    pathComponents.contains(kTrashDirectoryName)
  }
}
