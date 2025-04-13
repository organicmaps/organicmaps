final class RecentlyDeletedCategoriesViewModel: NSObject {

  typealias BookmarksManager = RecentlyDeletedCategoriesManager & BookmarksObservable

  enum Section: CaseIterable {
    struct Model: Equatable {
      var content: [RecentlyDeletedCategory]
    }

    case main
  }

  enum State {
    case searching
    case nothingSelected
    case someSelected
  }

  private var recentlyDeletedCategoriesManager: BookmarksManager
  private var dataSource: [Section.Model] = [] {
    didSet {
      if dataSource.isEmpty {
        onCategoriesIsEmpty?()
      }
    }
  }
  private(set) var state: State = .nothingSelected
  private(set) var filteredDataSource: [Section.Model] = []
  private(set) var selectedIndexPaths: [IndexPath] = []
  private(set) var searchText = String()

  var stateDidChange: ((State) -> Void)?
  var filteredDataSourceDidChange: (([Section.Model]) -> Void)?
  var onCategoriesIsEmpty: (() -> Void)?

  init(bookmarksManager: BookmarksManager) {
    self.recentlyDeletedCategoriesManager = bookmarksManager
    super.init()
    subscribeOnBookmarksManagerNotifications()
    fetchRecentlyDeletedCategories()
  }

  deinit {
    unsubscribeFromBookmarksManagerNotifications()
  }

  // MARK: - Private methods
  private func subscribeOnBookmarksManagerNotifications() {
    recentlyDeletedCategoriesManager.add(self)
  }

  private func unsubscribeFromBookmarksManagerNotifications() {
    recentlyDeletedCategoriesManager.remove(self)
  }

  private func updateState(to newState: State) {
    guard state != newState else { return }
    state = newState
    stateDidChange?(state)
  }

  private func updateFilteredDataSource(_ dataSource: [Section.Model]) {
    filteredDataSource = dataSource.filtered(using: searchText)
    filteredDataSourceDidChange?(filteredDataSource)
  }

  private func updateSelectionAtIndexPath(_ indexPath: IndexPath, isSelected: Bool) {
    if isSelected {
      updateState(to: .someSelected)
    } else {
      let allDeselected = dataSource.allSatisfy { $0.content.isEmpty }
      updateState(to: allDeselected ? .nothingSelected : .someSelected)
    }
  }

  private func removeCategories(at indexPaths: [IndexPath], completion: ([URL]) -> Void) {
    var fileToRemoveURLs: [URL]
    if indexPaths.isEmpty {
      // Remove all without selection.
      fileToRemoveURLs = dataSource.flatMap { $0.content.map { $0.fileURL } }
      dataSource.removeAll()
    } else {
      fileToRemoveURLs = [URL]()
      indexPaths.forEach { [weak self] indexPath in
        guard let self else { return }
        let fileToRemoveURL = self.filteredDataSource[indexPath.section].content[indexPath.row].fileURL
        self.dataSource[indexPath.section].content.removeAll { $0.fileURL == fileToRemoveURL }
        fileToRemoveURLs.append(fileToRemoveURL)
      }
    }
    updateFilteredDataSource(dataSource)
    updateState(to: .nothingSelected)
    completion(fileToRemoveURLs)
  }

  private func removeSelectedCategories(completion: ([URL]) -> Void) {
    let removeAll = selectedIndexPaths.isEmpty || selectedIndexPaths.count == dataSource.flatMap({ $0.content }).count
    removeCategories(at: removeAll ? [] : selectedIndexPaths, completion: completion)
    selectedIndexPaths.removeAll()
    updateState(to: .nothingSelected)
  }
}

// MARK: - Public methods
extension RecentlyDeletedCategoriesViewModel {
  func fetchRecentlyDeletedCategories() {
    let categories = recentlyDeletedCategoriesManager.getRecentlyDeletedCategories()
    guard !categories.isEmpty else { return }
    dataSource = [Section.Model(content: categories)]
    updateFilteredDataSource(dataSource)
  }

  func deleteCategory(at indexPath: IndexPath) {
    removeCategories(at: [indexPath]) { recentlyDeletedCategoriesManager.deleteRecentlyDeletedCategory(at: $0) }
  }

  func deleteSelectedCategories() {
    removeSelectedCategories { recentlyDeletedCategoriesManager.deleteRecentlyDeletedCategory(at: $0) }
  }

  func recoverCategory(at indexPath: IndexPath) {
    removeCategories(at: [indexPath]) { recentlyDeletedCategoriesManager.recoverRecentlyDeletedCategories(at: $0) }
  }

  func recoverSelectedCategories() {
    removeSelectedCategories { recentlyDeletedCategoriesManager.recoverRecentlyDeletedCategories(at: $0) }
  }

  func startSelecting() {
    updateState(to: .nothingSelected)
  }

  func selectCategory(at indexPath: IndexPath) {
    selectedIndexPaths.append(indexPath)
    updateState(to: .someSelected)
  }

  func deselectCategory(at indexPath: IndexPath) {
    selectedIndexPaths.removeAll { $0 == indexPath }
    if selectedIndexPaths.isEmpty {
      updateState(to: state == .searching ? .searching : .nothingSelected)
    }
  }

  func selectAllCategories() {
    selectedIndexPaths = dataSource.enumerated().flatMap { sectionIndex, section in
      section.content.indices.map { IndexPath(row: $0, section: sectionIndex) }
    }
    updateState(to: .someSelected)
  }

  func deselectAllCategories() {
    selectedIndexPaths.removeAll()
    updateState(to: state == .searching ? .searching : .nothingSelected)
  }

  func cancelSelecting() {
    selectedIndexPaths.removeAll()
    updateState(to: .nothingSelected)
  }

  func startSearching() {
    updateState(to: .searching)
  }

  func cancelSearching() {
    searchText.removeAll()
    selectedIndexPaths.removeAll()
    updateFilteredDataSource(dataSource)
    updateState(to: .nothingSelected)
  }

  func search(_ searchText: String) {
    updateState(to: .searching)
    guard !searchText.isEmpty else {
      cancelSearching()
      return
    }
    self.searchText = searchText
    updateFilteredDataSource(dataSource)
  }
}

// MARK: - BookmarksObserver

extension RecentlyDeletedCategoriesViewModel: BookmarksObserver {
  func onBookmarksLoadFinished() {
    fetchRecentlyDeletedCategories()
  }

  func onRecentlyDeletedBookmarksCategoriesChanged() {
    fetchRecentlyDeletedCategories()
  }
}


private extension Array where Element == RecentlyDeletedCategoriesViewModel.Section.Model {
  func filtered(using searchText: String) -> [Element] {
    let filteredArray = map { section in
      let filteredContent = section.content.filter {
        guard !searchText.isEmpty else { return true }
        return $0.title.localizedCaseInsensitiveContains(searchText)
      }
      return RecentlyDeletedCategoriesViewModel.Section.Model(content: filteredContent)
    }
    return filteredArray
  }
}
