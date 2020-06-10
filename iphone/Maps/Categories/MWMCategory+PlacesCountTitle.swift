import Foundation

extension BookmarkGroup {
  @objc func placesCountTitle() -> String {
    if (bookmarksCount > 0 && trackCount > 0) ||
      (bookmarksCount == 0 && trackCount == 0) {
       return String(format: L("objects"), bookmarksCount + trackCount)
    } else if (bookmarksCount > 0) {
      return String(format: L("bookmarks_places"), bookmarksCount)
    } else {
       return String(format: L("tracks"), trackCount)
    }
  }
}
