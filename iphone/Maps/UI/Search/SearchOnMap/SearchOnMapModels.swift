enum SearchOnMap {
  struct ViewModel: Equatable {
    enum ContentState: Equatable {
      case historyAndCategory
      case results(SearchResults)
      case noResults
      case searching
    }

    var isTyping: Bool
    var searchingText: String?
    var contentState: ContentState
    var presentationStep: ModalScreenPresentationStep
  }

  struct SearchResults: Equatable {
    let results: [SearchResult]
    let hasPartialMatch: Bool
    let isEmpty: Bool
    let count: Int
    let suggestionsCount: Int

    init(_ results: [SearchResult]) {
      self.results = results
      self.hasPartialMatch = !results.allSatisfy { $0.highlightRanges.isEmpty }
      self.isEmpty = results.isEmpty
      self.count = results.count
      self.suggestionsCount = results.filter { $0.itemType == .suggestion }.count
    }
  }

  struct SearchText {
    let text: String
    let locale: String

    init(_ text: String, locale: String? = nil) {
      self.text = text
      self.locale = locale ?? AppInfo.shared().languageId
    }
  }

  enum Request {
    case viewDidLoad
    case didStartDraggingSearch
    case didStartDraggingMap
    case didStartTyping
    case didType(SearchText)
    case didSelectText(SearchText, isCategory: Bool)
    case didSelectResult(SearchResult, atIndex: Int, withSearchText: SearchText)
    case searchButtonDidTap(SearchText)
    case clearButtonDidTap
    case didSelectPlaceOnMap
    case didDeselectPlaceOnMap
    case didUpdatePresentationStep(ModalScreenPresentationStep)
    case openSearch
    case hideSearch
    case closeSearch
  }

  enum Response: Equatable {
    case startSearching
    case showOnTheMap
    case setIsTyping(Bool)
    case showHistoryAndCategory
    case showResults(SearchResults, isSearchCompleted: Bool = false)
    case selectText(String?)
    case clearSearch
    case setSearchScreenHidden(Bool)
    case setSearchScreenCompact
    case updatePresentationStep(ModalScreenPresentationStep)
    case close
  }
}

extension SearchOnMap.SearchResults {
  static let empty = SearchOnMap.SearchResults([])

  subscript(index: Int) -> SearchResult {
    results[index]
  }
}
