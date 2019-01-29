protocol SearchCategoriesViewControllerDelegate: AnyObject {
  func categoriesViewController(_ viewController: SearchCategoriesViewController,
                                didSelect category: String)
}

final class SearchCategoriesViewController: MWMTableViewController {
  private weak var delegate: SearchCategoriesViewControllerDelegate?
  private let categories: [String]
  private let rutaxi: Bool
  private static let rutaxiIndex = 6
  
  init(frameworkHelper: MWMSearchFrameworkHelper, delegate: SearchCategoriesViewControllerDelegate?) {
    self.delegate = delegate
    self.categories = frameworkHelper.searchCategories()
    self.rutaxi = frameworkHelper.hasMegafonCategoryBanner()
    super.init(nibName: nil, bundle: nil)
  }
  
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    tableView.register(cellClass: SearchCategoryCell.self)
    tableView.register(cellClass: SearchBannerCell.self)
    tableView.separatorStyle = .none
  }
  
  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return categories.count + (rutaxi ? 1 : 0)
  }
  
  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    if rutaxi && (indexPath.row == SearchCategoriesViewController.rutaxiIndex) {
      let cell = tableView.dequeueReusableCell(cell: SearchBannerCell.self, indexPath: indexPath)
      cell.delegate = self
      return cell
    }
    
    let cell = tableView.dequeueReusableCell(cell: SearchCategoryCell.self, indexPath: indexPath)
    cell.update(with: category(at: indexPath))
    return cell
  }
  
  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    let selectedCategory = category(at: indexPath)
    delegate?.categoriesViewController(self, didSelect: selectedCategory)
    
    Statistics.logEvent(kStatEventName(kStatSearch, kStatSelectResult),
                        withParameters: [kStatValue : selectedCategory, kStatScreen : kStatCategories])
  }
  
  func category(at indexPath: IndexPath) -> String {
    let index = indexPath.row
    if rutaxi && (index > SearchCategoriesViewController.rutaxiIndex) {
      return categories[index - 1]
    } else {
      return categories[index]
    }
  }
}

extension SearchCategoriesViewController: SearchBannerCellDelegate {
  func cellDidPressAction(_ cell: SearchBannerCell) {
    guard let url = URL(string: "https://go.onelink.me/2944814706/86db6339") else {
      assertionFailure()
      return
    }
    UIApplication.shared.open(url)
  }
  
  func cellDidPressClose(_ cell: SearchBannerCell) {
    MapViewController.shared().showRemoveAds()
  }
}
