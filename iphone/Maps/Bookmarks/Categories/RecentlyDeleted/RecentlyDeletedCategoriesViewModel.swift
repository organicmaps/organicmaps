struct RecentlyDeletedCategory: Equatable {
  let fileName: String
  let fileURL: URL
  let deletionDate: TimeInterval
}

private struct CategoryTextFilter {
  static let empty = CategoryTextFilter(searchText: "")

  var searchText: String

  func isMatch(for category: RecentlyDeletedCategory) -> Bool {
    guard !searchText.isEmpty else { return true }
    return category.fileName.localizedCaseInsensitiveContains(searchText)
  }
}

final class RecentlyDeletedCategoriesViewModel {
  struct State {
    struct SectionModel: Equatable {
      var categories: [RecentlyDeletedCategory]
    }

    enum InteractionMode {
      case normal
      case searching
      case editingAndNothingSelected
      case editingAndSomeSelected
    }

    var filteredDataSource: [SectionModel] = []
    var unfilteredDataSource: [SectionModel] = []
    var interactionMode: InteractionMode = .normal
    var selectedIndexPaths: [IndexPath] = []
    fileprivate var filter: CategoryTextFilter = .empty
  }

  enum Action {
    case fetchRecentlyDeletedCategories
    case delete(at: IndexPath)
    case deleteSelected
    case recover(at: IndexPath)
    case recoverSelected
    case startSelecting
    case select(at: IndexPath)
    case deselect(at: IndexPath)
    case selectAll
    case deselectAll
    case cancelSelecting
    case startSearching
    case cancelSearching
    case search(String)
  }

  enum Event {
    case updateCategories([State.SectionModel])
    case removeCategory(at: [IndexPath])
    case updateInteractionMode(State.InteractionMode)
  }

  private var bookmarksManager: any RecentlyDeletedCategoriesManager
  private(set) var state = State()

  var didReceiveEvent: ((Event) -> Void)?

  init(bookmarksManager: RecentlyDeletedCategoriesManager = BookmarksManager.shared()) {
    self.bookmarksManager = bookmarksManager
  }

  // MARK: - Public methods
  func handleAction(_ action: Action) {
    let newState = reduce(from: state, action: action)
    let events = events(from: state, to: newState, action: action)
    state = newState
    events.forEach { didReceiveEvent?($0) }
  }
}

// MARK: - Private methods
private extension RecentlyDeletedCategoriesViewModel {
  func reduce(from: State, action: Action) -> State {
    switch action {
    case .fetchRecentlyDeletedCategories: return fetchRecentlyDeletedCategories()
    case .delete(let indexPath): return removeCategories(state: from, at: [indexPath]) { bookmarksManager.deleteRecentlyDeletedCategory(at: $0) }
    case .deleteSelected: return removeSelectedCategories(state: from) { bookmarksManager.deleteRecentlyDeletedCategory(at: $0) }
    case .recover(let indexPath): return removeCategories(state: from, at: [indexPath]) { bookmarksManager.recoverRecentlyDeletedCategories(at: $0) }
    case .recoverSelected: return removeSelectedCategories(state: from) { bookmarksManager.recoverRecentlyDeletedCategories(at: $0) }
    case .startSelecting: return startSelecting(state: from)
    case .select(let indexPath): return selectCategory(state: from, at: indexPath)
    case .deselect(let indexPath): return deselectCategory(state: from, at: indexPath)
    case .selectAll: return selectAllCategories(state: from)
    case .deselectAll: return deselectAllCategories(state: from)
    case .cancelSelecting: return cancelSelecting(state: from)
    case .startSearching: return startSearching(state: from)
    case .cancelSearching: return cancelSearching(state: from)
    case .search(let searchText): return search(state: from, searchText: searchText)
    }
  }

  func events(from state: State, to newSate: State, action: Action) -> [Event] {
    var events = [Event]()
    switch action {
    case .recover(let indexPath):
      events.append(.removeCategory(at: [indexPath]))
    case .delete(let indexPath):
      events.append(.removeCategory(at: [indexPath]))
    default:
      if state.filteredDataSource != newSate.filteredDataSource {
        events.append(.updateCategories(newSate.filteredDataSource))
      }
    }
    if state.interactionMode != newSate.interactionMode {
      events.append(.updateInteractionMode(newSate.interactionMode))
    }
    return events
  }

  func fetchRecentlyDeletedCategories() -> State {
    let recentlyDeletedCategoryURLs = bookmarksManager.getRecentlyDeletedCategories()
    let categories = recentlyDeletedCategoryURLs.map { fileUrl in
      let fileName = fileUrl.lastPathComponent
      // TODO: remove this code with cpp
      let deletionDate = (try! fileUrl.resourceValues(forKeys: [.creationDateKey]).creationDate ?? Date()).timeIntervalSince1970
      return RecentlyDeletedCategory(fileName: fileName, fileURL: fileUrl, deletionDate: deletionDate)
    }
    let dataSource = [State.SectionModel(categories: categories)]
    let state = State(filteredDataSource: dataSource,
                      unfilteredDataSource: dataSource,
                      interactionMode: .normal,
                      selectedIndexPaths: [],
                      filter: .empty)
    return state
  }

  func startSelecting(state: State) -> State {
    var newState = state
    newState.interactionMode = .editingAndNothingSelected
    return newState
  }

  func selectCategory(state: State, at indexPath: IndexPath) -> State {
    var newState = state
    newState.selectedIndexPaths.append(indexPath)
    newState.interactionMode = .editingAndSomeSelected
    return newState
  }

  func deselectCategory(state: State, at indexPath: IndexPath) -> State {
    var newState = state
    newState.selectedIndexPaths.removeAll { $0 == indexPath }
    if newState.selectedIndexPaths.isEmpty {
      newState.interactionMode = .editingAndNothingSelected
    }
    return newState
  }

  func selectAllCategories(state: State) -> State {
    var newState = state
    newState.selectedIndexPaths = state.unfilteredDataSource.enumerated().flatMap { sectionIndex, section in
      section.categories.indices.map { IndexPath(row: $0, section: sectionIndex) }
    }
    newState.interactionMode = .editingAndSomeSelected
    return newState
  }

  func deselectAllCategories(state: State) -> State {
    var newState = state
    newState.selectedIndexPaths.removeAll()
    newState.interactionMode = .editingAndNothingSelected
    return newState
  }

  func cancelSelecting(state: State) -> State {
    var newState = state
    newState.selectedIndexPaths.removeAll()
    newState.interactionMode = .normal
    return newState
  }

  func startSearching(state: State) -> State {
    var newState = state
    newState.interactionMode = .searching
    return newState
  }

  func cancelSearching(state: State) -> State {
    var newState = state
    newState.interactionMode = .normal
    newState.filteredDataSource = state.unfilteredDataSource
    newState.selectedIndexPaths.removeAll()
    newState.filter = .empty
    return newState
  }

  func search(state: State, searchText: String) -> State {
    var newState = state
    newState.interactionMode = .searching
    newState.filter.searchText = searchText
    guard !searchText.isEmpty else {
      return cancelSearching(state: newState)
    }
    newState.filterDataSource()
    return newState
  }

  func removeCategories(state: State, at indexPaths: [IndexPath], completion: ([URL]) -> Void) -> State {
    var newState = state
    var fileToRemoveURLs: [URL]
    if indexPaths.isEmpty {
      // Remove all without selection.
      fileToRemoveURLs = newState.unfilteredDataSource.flatMap { $0.categories.map { $0.fileURL } }
      newState.unfilteredDataSource.removeAll()
    } else {
      fileToRemoveURLs = [URL]()
      indexPaths.forEach { indexPath in
        let fileToRemoveURL = newState.filteredDataSource[indexPath.section].categories[indexPath.row].fileURL
        newState.unfilteredDataSource[indexPath.section].categories.removeAll { $0.fileURL == fileToRemoveURL }
        fileToRemoveURLs.append(fileToRemoveURL)
      }
    }
    newState.filterDataSource()
    newState.selectedIndexPaths.removeAll()
    newState.interactionMode = .normal
    completion(fileToRemoveURLs)
    return newState
  }

  func removeSelectedCategories(state: State, completion: ([URL]) -> Void) -> State {
    let shouldRemoveAllCategories = state.selectedIndexPaths.isEmpty || state.selectedIndexPaths.count == state.unfilteredDataSource.flatMap({ $0.categories }).count
    return removeCategories(state: state, at: shouldRemoveAllCategories ? [] : state.selectedIndexPaths, completion: completion)
  }
}

private extension RecentlyDeletedCategoriesViewModel.State {
  mutating func filterDataSource() {
    filteredDataSource = unfilteredDataSource.filtered(using: filter)
  }
}

private extension Array where Element == RecentlyDeletedCategoriesViewModel.State.SectionModel {
  func filtered(using filter: CategoryTextFilter) -> [Element] {
    let filteredArray = map { section in
      let filteredCategories = section.categories.filter { filter.isMatch(for: $0) }
      return Element(categories: filteredCategories)
    }
    return filteredArray
  }
}
