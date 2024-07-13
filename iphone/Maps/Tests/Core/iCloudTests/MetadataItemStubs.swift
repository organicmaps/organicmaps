@testable import Organic_Maps__Debug_

extension LocalMetadataItem {
  static func stub(fileName: String,
                   lastModificationDate: TimeInterval,
                   isDeleted: Bool = false) -> LocalMetadataItem {
    let name = generateName(from: fileName, isDeleted: isDeleted)
    let url = generateURL(from: name)
    let item = LocalMetadataItem(fileName: name,
                                 fileUrl: url,
                                 fileSize: 0,
                                 contentType: "",
                                 creationDate: Date().timeIntervalSince1970,
                                 lastModificationDate: lastModificationDate,
                                 isDeleted: isDeleted)
    return item

  }
}

extension CloudMetadataItem {
  static func stub(fileName: String,
                   lastModificationDate: TimeInterval,
                   isTrashed: Bool = false,
                   isDeleted: Bool = false,
                   isDownloaded: Bool = true,
                   hasUnresolvedConflicts: Bool = false) -> CloudMetadataItem {
    let name = generateName(from: fileName, isDeleted: isDeleted)
    let url = generateURL(from: name)
    let item = CloudMetadataItem(fileName: name,
                                 fileUrl: url,
                                 fileSize: 0,
                                 contentType: "",
                                 isDownloaded: isDownloaded,
                                 creationDate: Date().timeIntervalSince1970,
                                 lastModificationDate: lastModificationDate,
                                 isDeleted: isDeleted,
                                 isRemoved: isTrashed,
                                 downloadingError: nil,
                                 uploadingError: nil,
                                 hasUnresolvedConflicts: hasUnresolvedConflicts)
    return item
  }
}

fileprivate func generateName(from fileName: String, isDeleted: Bool) -> String {
  isDeleted ? fileName.appending(".deleted") : fileName
}

fileprivate func generateURL(from fileName: String) -> URL {
  URL(fileURLWithPath: "url://\(fileName)")
}
