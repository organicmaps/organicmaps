class AvailableMapsDataSource {
  struct Const {
    static let locationArrow = "➤"
  }

  private let parentCountryId: String?

  private var sections: [String]?
  private var sectionsContent: [String: [String]]?
  private var nearbySection: [String]?

  fileprivate var searching = false
  fileprivate lazy var searchDataSource: IDownloaderDataSource = {
    SearchMapsDataSource()
  }()

  init(_ parentCountryId: String? = nil, location: CLLocationCoordinate2D? = nil) {
    self.parentCountryId = parentCountryId
    let countryIds: [String]
    if let parentCountryId = parentCountryId {
      countryIds = Storage.shared().allCountries(withParent: parentCountryId)
    } else {
      countryIds = Storage.shared().allCountries()
    }
    configSections(countryIds, location: location)
  }

  private func configSections(_ countryIds: [String], location: CLLocationCoordinate2D?) {
    let countries = countryIds.map {
      CountryIdAndName(countryId: $0, name: Storage.shared().name(forCountry: $0))
    }.sorted {
      $0.countryName.compare($1.countryName) == .orderedAscending
    }

    sections = []
    sectionsContent = [:]

    if let location = location, let nearbySection = Storage.shared().nearbyAvailableCountries(location) {
      sections?.append(Const.locationArrow)
      sectionsContent![Const.locationArrow] = nearbySection
    }

    for country in countries {
      let section = parentCountryId == nil ? String(country.countryName.prefix(1)) : L("downloader_available_maps")
      if sections!.last != section {
        sections!.append(section)
        sectionsContent![section] = []
      }

      var sectionCountries = sectionsContent![section]
      sectionCountries?.append(country.countryId)
      sectionsContent![section] = sectionCountries
    }
  }
}

extension AvailableMapsDataSource: IDownloaderDataSource {
  var isEmpty: Bool {
    searching ? searchDataSource.isEmpty : false
  }

  var title: String {
    guard let parentCountryId = parentCountryId else {
      return L("download_maps")
    }
    return Storage.shared().name(forCountry: parentCountryId)
  }

  var isRoot: Bool {
    parentCountryId == nil
  }

  var isSearching: Bool {
    searching
  }

  func getParentCountryId() -> String {
    if parentCountryId != nil {
      return parentCountryId!
    }
    return Storage.shared().getRootId()
  }

  func parentAttributes() -> MapNodeAttributes {
    guard let parentId = parentCountryId else {
      return Storage.shared().attributesForRoot()
    }
    return Storage.shared().attributes(forCountry: parentId)
  }

  func numberOfSections() -> Int {
    searching ? searchDataSource.numberOfSections() : (sections?.count ?? 0)
  }

  func numberOfItems(in section: Int) -> Int {
    if searching {
      return searchDataSource.numberOfItems(in: section)
    }
    let index = sections![section]
    return sectionsContent![index]!.count
  }

  func item(at indexPath: IndexPath) -> MapNodeAttributes {
    if searching {
      return searchDataSource.item(at: indexPath)
    }
    let sectionIndex = sections![indexPath.section]
    let sectionItems = sectionsContent![sectionIndex]
    let countryId = sectionItems![indexPath.item]
    return Storage.shared().attributes(forCountry: countryId)
  }

  func matchedName(at indexPath: IndexPath) -> String? {
    searching ? searchDataSource.matchedName(at: indexPath) : nil
  }

  func title(for section: Int) -> String {
    if searching {
      return searchDataSource.title(for: section)
    }
    let title = sections![section]
    if title == Const.locationArrow {
      return L("downloader_near_me_subtitle")
    }
    return title
  }

  func indexTitles() -> [String]? {
    if searching {
      return nil
    }
    if parentCountryId != nil {
      return nil
    }
    return sections
  }

  func dataSourceFor(_ childId: String) -> IDownloaderDataSource {
    searching ? searchDataSource.dataSourceFor(childId) : AvailableMapsDataSource(childId)
  }

  func reload(_ completion: () -> Void) {
    if searching {
      searchDataSource.reload(completion)
    }
    // do nothing.
    completion()
  }

  func search(_ query: String, locale: String, update: @escaping (Bool) -> Void) {
    if query.isEmpty {
      cancelSearch()
      update(true)
      return
    }
    searchDataSource.search(query, locale: locale) { [weak self] (finished) in
      if finished {
        self?.searching = true
        update(finished)
      }
    }
  }

  func cancelSearch() {
    searching = false
    searchDataSource.cancelSearch()
  }
}
