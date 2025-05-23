@objcMembers
final class SearchQuery: NSObject {
  let text: String
  let locale: String
  let source: SearchTextSource

  init(_ text: String, locale: String? = nil, source: SearchTextSource) {
    self.text = text
    self.locale = locale ?? AppInfo.shared().languageId
    self.source = source
  }
}

enum SearchOnMap {
  struct ViewModel: Equatable {
    enum Content: Equatable {
      case historyAndCategory
      case results(SearchResults)
      case noResults
      case searching
    }

    var isTyping: Bool
    var skipSuggestions: Bool
    var searchingText: String?
    var contentState: Content
    var presentationStep: ModalPresentationStep
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

  enum Request {
    case openSearch
    case hideSearch
    case closeSearch
    case didStartDraggingSearch
    case didStartDraggingMap
    case didStartTyping
    case didType(SearchQuery)
    case didSelect(SearchQuery)
    case didSelectResult(SearchResult, withQuery: SearchQuery)
    case searchButtonDidTap(SearchQuery)
    case clearButtonDidTap
    case didSelectPlaceOnMap
    case didDeselectPlaceOnMap
    case didUpdatePresentationStep(ModalPresentationStep)
  }

  enum Response: Equatable {
    case startSearching
    case showOnTheMap
    case setIsTyping(Bool)
    case showHistoryAndCategory
    case showResults(SearchResults, isSearchCompleted: Bool = false)
    case selectQuery(SearchQuery)
    case clearSearch
    case setSearchScreenHidden(Bool)
    case setSearchScreenCompact
    case updatePresentationStep(ModalPresentationStep)
    case close
    case none
  }
}

extension SearchOnMap.SearchResults {
  static let empty = SearchOnMap.SearchResults([])

  subscript(index: Int) -> SearchResult {
    results[index]
  }

  mutating func skipSuggestions() {
    self = SearchOnMap.SearchResults(results.filter { $0.itemType != .suggestion })
  }
}

extension SearchOnMap.ViewModel {
  static let initial = SearchOnMap.ViewModel(isTyping: false,
                                             skipSuggestions: false,
                                             searchingText: nil,
                                             contentState: .historyAndCategory,
                                             presentationStep: .fullScreen)
}
