enum SearchOnMap {
  struct ViewModel: Equatable {
    enum ContentState: Equatable {
      case historyAndCategory
      case results([SearchResult])
      case noResults
      case searching
    }

    var isTyping: Bool
    var searchingText: String?
    var contentState: ContentState
    var presentationStep: ModalScreenPresentationStep
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
    case closeSearch
  }

  enum Response: Equatable {
    case startSearching
    case showOnTheMap
    case setIsTyping(Bool)
    case showHistoryAndCategory
    case showResults([SearchResult], isSearchCompleted: Bool = false)
    case selectText(String?)
    case clearSearch
    case setSearchScreenHidden(Bool)
    case setSearchScreenCompact
    case updatePresentationStep(ModalScreenPresentationStep)
    case close
  }
}
