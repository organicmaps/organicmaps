protocol MetadataItem: Equatable, Hashable {
  var fileName: String { get }
  var fileUrl: URL { get }
  var fileSize: Int { get }
  var contentType: String { get }
  var creationDate: TimeInterval { get }
  var lastModificationDate: TimeInterval { get }
}

struct CloudMetadataItem: MetadataItem {
  let fileName: String
  let fileUrl: URL
  let fileSize: Int
  let contentType: String
  var isDownloaded: Bool
  var downloadAmount: Double
  let creationDate: TimeInterval
  var lastModificationDate: TimeInterval
  var isInTrash: Bool
  let downloadingError: NSError?
  let uploadingError: NSError?
  let hasUnresolvedConflicts: Bool
}

extension CloudMetadataItem {
  init(metadataItem: NSMetadataItem) throws {
    guard let fileName = metadataItem.value(forAttribute: NSMetadataItemFSNameKey) as? String,
          let fileUrl = metadataItem.value(forAttribute: NSMetadataItemURLKey) as? URL,
          let fileSize = metadataItem.value(forAttribute: NSMetadataItemFSSizeKey) as? Int,
          let contentType = metadataItem.value(forAttribute: NSMetadataItemContentTypeKey) as? String,
          let downloadAmount = metadataItem.value(forAttribute: NSMetadataUbiquitousItemPercentDownloadedKey) as? Double,
          let downloadStatus = metadataItem.value(forAttribute: NSMetadataUbiquitousItemDownloadingStatusKey) as? String,
          let creationDate = (metadataItem.value(forAttribute: NSMetadataItemFSCreationDateKey) as? Date)?.timeIntervalSince1970.rounded(.down),
          let lastModificationDate = (metadataItem.value(forAttribute: NSMetadataItemFSContentChangeDateKey) as? Date)?.timeIntervalSince1970.rounded(.down),
          let hasUnresolvedConflicts = metadataItem.value(forAttribute: NSMetadataUbiquitousItemHasUnresolvedConflictsKey) as? Bool else {
      throw NSError(domain: "CloudMetadataItem", code: 0, userInfo: [NSLocalizedDescriptionKey: "Failed to initialize CloudMetadataItem from NSMetadataItem"])
    }
    self.fileName = fileName
    self.fileUrl = fileUrl
    self.fileSize = fileSize
    self.contentType = contentType
    self.isDownloaded = downloadStatus == NSMetadataUbiquitousItemDownloadingStatusCurrent
    self.downloadAmount = downloadAmount
    self.creationDate = creationDate
    self.lastModificationDate = lastModificationDate
    self.isInTrash = fileUrl.pathComponents.contains(kTrashDirectoryName)
    self.hasUnresolvedConflicts = hasUnresolvedConflicts
    self.downloadingError = metadataItem.value(forAttribute: NSMetadataUbiquitousItemDownloadingErrorKey) as? NSError
    self.uploadingError = metadataItem.value(forAttribute: NSMetadataUbiquitousItemUploadingErrorKey) as? NSError
  }
}

struct LocalMetadataItem: MetadataItem {
  let fileName: String
  let fileUrl: URL
  let fileSize: Int
  let contentType: String
  let creationDate: TimeInterval
  let lastModificationDate: TimeInterval
}

extension LocalMetadataItem {
  init(metadataItem: NSMetadataItem) throws {
    guard let fileName = metadataItem.value(forAttribute: NSMetadataItemFSNameKey) as? String,
          let fileUrl = metadataItem.value(forAttribute: NSMetadataItemURLKey) as? URL,
          let fileSize = metadataItem.value(forAttribute: NSMetadataItemFSSizeKey) as? Int,
          let contentType = metadataItem.value(forAttribute: NSMetadataItemContentTypeKey) as? String,
          let creationDate = (metadataItem.value(forAttribute: NSMetadataItemFSCreationDateKey) as? Date)?.timeIntervalSince1970.rounded(.down),
          let lastModificationDate = (metadataItem.value(forAttribute: NSMetadataItemFSContentChangeDateKey) as? Date)?.timeIntervalSince1970.rounded(.down) else {
      throw NSError(domain: "LocalMetadataItem", code: 0, userInfo: [NSLocalizedDescriptionKey: "Failed to initialize LocalMetadataItem from NSMetadataItem"])
    }
    self.fileName = fileName
    self.fileUrl = fileUrl
    self.fileSize = fileSize
    self.contentType = contentType
    self.creationDate = creationDate
    self.lastModificationDate = lastModificationDate
  }

  init(fileUrl: URL) throws {
    guard let fileSize = try? fileUrl.resourceValues(forKeys: [.fileSizeKey]).fileSize,
          let contentType = try? fileUrl.resourceValues(forKeys: [.typeIdentifierKey]).typeIdentifier,
          let creationDate = try? fileUrl.resourceValues(forKeys: [.creationDateKey]).creationDate?.timeIntervalSince1970.rounded(.down),
          let lastModificationDate = try? fileUrl.resourceValues(forKeys: [.contentModificationDateKey]).contentModificationDate?.timeIntervalSince1970.rounded(.down) else {
      throw NSError(domain: "LocalMetadataItem", code: 0, userInfo: [NSLocalizedDescriptionKey: "Failed to initialize LocalMetadataItem from URL"])
    }
    self.fileName = fileUrl.lastPathComponent
    self.fileUrl = fileUrl
    self.fileSize = fileSize
    self.contentType = contentType
    self.creationDate = creationDate
    self.lastModificationDate = lastModificationDate
  }

  func fileData() throws -> Data {
    try Data(contentsOf: fileUrl)
  }
}
