import Foundation

extension BookmarkGroup {
  @objc func placesCountTitle() -> String {
    let bookmarks = String(format: L("bookmarks_places"), bookmarksCount)
    let tracks = String(format: L("tracks"), trackCount)

    if (bookmarksCount == 0 && trackCount == 0) || (bookmarksCount > 0 && trackCount > 0) {
      return "\(bookmarks), \(tracks)"
    }

    if (bookmarksCount > 0) {
      return bookmarks
    }

    return tracks

  }
}
