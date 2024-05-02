@testable import Organic_Maps__Debug_

extension LocalMetadataItem {
  static func stub(fileName: String, 
                   lastModificationDate: TimeInterval) -> LocalMetadataItem {
    let item = LocalMetadataItem(fileName: fileName,
                                 fileUrl: URL(string: "url")!,
                                 fileSize: 0,
                                 contentType: "",
                                 creationDate: Date().timeIntervalSince1970,
                                 lastModificationDate: lastModificationDate)
    return item

  }
}

extension CloudMetadataItem {
  static func stub(fileName: String, 
                   lastModificationDate: TimeInterval,
                   isInTrash: Bool,
                   isDownloaded: Bool = true,
                   hasUnresolvedConflicts: Bool = false) -> CloudMetadataItem {
    let item = CloudMetadataItem(fileName: fileName,
                                 fileUrl: URL(string: "url")!,
                                 fileSize: 0,
                                 contentType: "",
                                 isDownloaded: isDownloaded,
                                 creationDate: Date().timeIntervalSince1970,
                                 lastModificationDate: lastModificationDate,
                                 isRemoved: isInTrash,
                                 downloadingError: nil,
                                 uploadingError: nil,
                                 hasUnresolvedConflicts: hasUnresolvedConflicts)
    return item
  }
}
