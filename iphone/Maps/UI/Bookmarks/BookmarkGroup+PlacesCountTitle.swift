import Foundation

extension BookmarkGroup {
  @objc func placesCountTitle() -> String {
    guard areaCount > 0 else { return bookmarksAndTracksCountTitle() }
    let areas = String(format: L("areas"), areaCount)
    guard bookmarksCount > 0 || trackCount > 0 else { return areas }
    return "\(bookmarksAndTracksCountTitle()), \(areas)"
  }

  private func bookmarksAndTracksCountTitle() -> String {
    let bookmarks = String(format: L("bookmarks_places"), bookmarksCount)
    let tracks = String(format: L("tracks"), trackCount)

    if (bookmarksCount == 0 && trackCount == 0) || (bookmarksCount > 0 && trackCount > 0) {
      return "\(bookmarks), \(tracks)"
    }

    if bookmarksCount > 0 {
      return bookmarks
    }

    return tracks
  }
}
