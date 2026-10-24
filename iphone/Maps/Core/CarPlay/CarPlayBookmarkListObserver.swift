import CarPlay

final class CarPlayBookmarkListObserver: NSObject, BookmarksObserver {
  private let templates: () -> [CPTemplate]
  private let popTo: (CPTemplate) -> Void
  private var isActive = true
  private var isRefreshScheduled = false

  init(templates: @escaping () -> [CPTemplate], popTo: @escaping (CPTemplate) -> Void) {
    self.templates = templates
    self.popTo = popTo
    super.init()
    BookmarksManager.shared().add(self)
  }

  func invalidate() {
    // Callback delivery can retain the observer until its autorelease pool drains.
    isActive = false
    BookmarksManager.shared().remove(self)
  }

  func onBookmarksLoadFinished() {
    refresh()
  }

  func onBookmarksChanged() {
    // Loading publishes partial categories; refresh that batch once it finishes.
    guard BookmarksManager.shared().areBookmarksLoaded() else { return }
    refresh()
  }

  /// Deletions must invalidate stale rows immediately, including during file loading.
  func onBookmarksCategoryDeleted(_: MWMMarkGroupID) {
    refresh()
  }

  func onBookmarksDeleted(_: [NSNumber]) {
    refresh()
  }

  func onTracksDeleted(_: [NSNumber]) {
    refresh()
  }

  func refresh() {
    guard isActive, !isRefreshScheduled else { return }
    isRefreshScheduled = true
    // Each bookmark row needs an address lookup, so rebuild only once for a burst of callbacks.
    DispatchQueue.main.async { [weak self] in
      guard let self, isActive else { return }
      isRefreshScheduled = false
      // Read the current stack after the queue hop: the connection or templates may have changed.
      let templates = self.templates()
      if let index = templates.firstIndex(where: ListTemplateBuilder.isDeletedBookmarkList), index > 0 {
        popTo(templates[index - 1])
      }
      templates.forEach(ListTemplateBuilder.refreshBookmarks(in:))
    }
  }
}
