class DownloadedMapsDataSource {
  private let parentCountryId: String?
  private var countryIds: [String]

  fileprivate var searching = false
  fileprivate lazy var searchDataSource: IDownloaderDataSource = {
    SearchMapsDataSource()
  }()

  init(_ parentId: String? = nil) {
    self.parentCountryId = parentId
    countryIds = DownloadedMapsDataSource.loadData(parentId)
  }

  private class func loadData(_ parentId: String?) -> [String] {
    let countryIds: [String]
    if let parentId = parentId {
      countryIds = Storage.downloadedCountries(withParent: parentId)
    } else {
      countryIds = Storage.downloadedCountries()
    }

    return countryIds.map {
      CountryIdAndName(countryId: $0, name: Storage.name(forCountry: $0))
    }.sorted {
      $0.countryName.compare($1.countryName) == .orderedAscending
    }.map {
      $0.countryId
    }
  }

  fileprivate func reloadData() {
    countryIds = DownloadedMapsDataSource.loadData(parentCountryId)
  }
}

extension DownloadedMapsDataSource: IDownloaderDataSource {
  var isEmpty: Bool {
    return searching ? searchDataSource.isEmpty : countryIds.isEmpty
  }
  
  var title: String {
    guard let parentCountryId = parentCountryId else {
      return L("downloader_my_maps_title")
    }
    return Storage.name(forCountry: parentCountryId)
  }

  var isRoot: Bool {
    parentCountryId == nil
  }

  var isSearching: Bool {
    searching
  }

  func parentAttributes() -> MapNodeAttributes {
    guard let parentId = parentCountryId else {
      return Storage.attributesForRoot()
    }
    return Storage.attributes(forCountry: parentId)
  }

  func numberOfSections() -> Int {
    searching ? searchDataSource.numberOfSections() : 1
  }

  func numberOfItems(in section: Int) -> Int {
    searching ? searchDataSource.numberOfItems(in: section) : countryIds.count
  }

  func item(at indexPath: IndexPath) -> MapNodeAttributes {
    if searching {
      return searchDataSource.item(at: indexPath)
    }
    guard indexPath.section == 0 else { fatalError() }
    let countryId = countryIds[indexPath.item]
    return Storage.attributes(forCountry: countryId)
  }

  func matchedName(at indexPath: IndexPath) -> String? {
    searching ? searchDataSource.matchedName(at: indexPath) : nil
  }

  func title(for section: Int) -> String {
    if searching {
      return searchDataSource.title(for: section)
    }
    if let parentCountryId = parentCountryId {
      let attributes = Storage.attributes(forCountry: parentCountryId)
      return Storage.name(forCountry: parentCountryId) + " (\(formattedSize(attributes.downloadedSize)))"
    }
    let attributes = Storage.attributesForRoot()
    return L("downloader_downloaded_subtitle") + " (\(formattedSize(attributes.downloadedSize)))"
  }

  func indexTitles() -> [String]? {
    nil
  }

  func dataSourceFor(_ childId: String) -> IDownloaderDataSource {
    searching ? searchDataSource.dataSourceFor(childId) : DownloadedMapsDataSource(childId)
  }

  func reload(_ completion: () -> Void) {
    if searching {
      searchDataSource.reload(completion)
      return
    }
    reloadData()
    completion()
  }

  func search(_ query: String, locale: String, update: @escaping (Bool) -> Void) {
    if query.isEmpty {
      cancelSearch()
      update(true)
      return
    }
    searching = true
    searchDataSource.search(query, locale: locale, update: update)
  }

  func cancelSearch() {
    searching = false
    searchDataSource.cancelSearch()
  }
}
