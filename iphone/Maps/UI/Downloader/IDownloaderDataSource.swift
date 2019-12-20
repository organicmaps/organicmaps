protocol IDownloaderDataSource {
  var isEmpty: Bool { get }
  var title: String { get }
  var isRoot: Bool { get }
  var isSearching: Bool { get }
  func parentAttributes() -> MapNodeAttributes
  func numberOfSections() -> Int
  func numberOfItems(in section: Int) -> Int
  func item(at indexPath: IndexPath) -> MapNodeAttributes
  func matchedName(at indexPath: IndexPath) -> String?
  func title(for section: Int) -> String
  func indexTitles() -> [String]?
  func dataSourceFor(_ childId: String) -> IDownloaderDataSource
  func reload(_ completion: () -> Void)
  func search(_ query: String, locale: String, update: @escaping (_ completed: Bool) -> Void)
  func cancelSearch()
}
