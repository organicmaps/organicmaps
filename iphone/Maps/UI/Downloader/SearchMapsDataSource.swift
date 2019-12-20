class SearchMapsDataSource {
  fileprivate var searchResults: [MapSearchResult] = []
  fileprivate var searchId = 0
}

extension SearchMapsDataSource: IDownloaderDataSource {
  var isEmpty: Bool {
    searchResults.isEmpty
  }

  var title: String {
    ""
  }

  var isRoot: Bool {
    true
  }

  var isSearching: Bool {
    true
  }

  func numberOfSections() -> Int {
    1
  }

  func parentAttributes() -> MapNodeAttributes {
    return Storage.attributesForRoot()
  }

  func numberOfItems(in section: Int) -> Int {
    searchResults.count
  }

  func item(at indexPath: IndexPath) -> MapNodeAttributes {
    Storage.attributes(forCountry: searchResults[indexPath.item].countryId)
  }

  func matchedName(at indexPath: IndexPath) -> String? {
    searchResults[indexPath.item].matchedName
  }

  func title(for section: Int) -> String {
    L("downloader_search_results")
  }

  func indexTitles() -> [String]? {
    nil
  }

  func dataSourceFor(_ childId: String) -> IDownloaderDataSource {
    AvailableMapsDataSource(childId)
  }

  func reload(_ completion: () -> Void) {
    completion()
  }

  func search(_ query: String, locale: String, update: @escaping (Bool) -> Void) {
    searchId += 1
    FrameworkHelper.search(inDownloader: query, inputLocale: locale) { [weak self, searchId] (results, finished) in
      self?.searchResults = results
      if searchId != self?.searchId {
        return
      }
      if results.count > 0 || finished {
        update(finished)
      }
    }
  }

  func cancelSearch() {
    self.searchResults = []
  }
}
