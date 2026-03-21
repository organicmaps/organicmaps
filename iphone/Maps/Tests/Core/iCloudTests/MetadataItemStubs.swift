@testable import Organic_Maps__Debug_

extension LocalMetadataItem {
  static func stub(fileName: String,
                   lastModificationDate: TimeInterval) -> LocalMetadataItem {
    LocalMetadataItem(fileName: fileName,
                      fileUrl: URL(string: "url")!,
                      lastModificationDate: lastModificationDate)
  }
}

extension CloudMetadataItem {
  static func stub(fileName: String,
                   lastModificationDate: TimeInterval,
                   isDownloaded: Bool = true,
                   percentDownloaded: NSNumber = 100.0,
                   hasUnresolvedConflicts: Bool = false) -> CloudMetadataItem {
    CloudMetadataItem(fileName: fileName,
                      fileUrl: URL(string: "url")!,
                      isDownloaded: isDownloaded,
                      percentDownloaded: percentDownloaded,
                      lastModificationDate: lastModificationDate,
                      downloadingError: nil,
                      uploadingError: nil,
                      hasUnresolvedConflicts: hasUnresolvedConflicts)
  }
}
