protocol SearchCategoriesViewControllerDelegate: AnyObject {
  func categoriesViewController(_ viewController: SearchCategoriesViewController,
                                didSelect category: String)
}

final class SearchCategoriesViewController: MWMTableViewController {
  private weak var delegate: SearchCategoriesViewControllerDelegate?
  private let categories: [String]
  private let showCitymobilBanner: Bool
  private let bannerUrl: URL?
  private var bannerShown = false
  private static let citymobilIndex = 6
  
  init(frameworkHelper: MWMSearchFrameworkHelper, delegate: SearchCategoriesViewControllerDelegate?) {
    self.delegate = delegate
    categories = frameworkHelper.searchCategories()
    bannerUrl = frameworkHelper.citymobilBannerUrl()
    showCitymobilBanner = bannerUrl != nil
    super.init(nibName: nil, bundle: nil)
  }
  
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    tableView.registerNib(cellClass: SearchCategoryCell.self)
    tableView.registerNib(cellClass: SearchBannerCell.self)
    tableView.separatorStyle = .none
    tableView.keyboardDismissMode = .onDrag
  }
  
  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return categories.count + (showCitymobilBanner ? 1 : 0)
  }
  
  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    if showCitymobilBanner && (indexPath.row == SearchCategoriesViewController.citymobilIndex) {
      let cell = tableView.dequeueReusableCell(cell: SearchBannerCell.self, indexPath: indexPath)
      cell.delegate = self
      if (!bannerShown) {
        bannerShown = true;
        Statistics.logEvent(kStatSearchSponsoredShow);
      }
      return cell
    }
    
    let cell = tableView.dequeueReusableCell(cell: SearchCategoryCell.self, indexPath: indexPath)
    cell.update(with: category(at: indexPath))
    return cell
  }
  
  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    if showCitymobilBanner && (indexPath.row == SearchCategoriesViewController.citymobilIndex) {
      openBanner()
      return
    }
    let selectedCategory = category(at: indexPath)
    delegate?.categoriesViewController(self, didSelect: selectedCategory)
    
    Statistics.logEvent(kStatEventName(kStatSearch, kStatSelectResult),
                        withParameters: [kStatValue : selectedCategory, kStatScreen : kStatCategories])
  }
  
  func category(at indexPath: IndexPath) -> String {
    let index = indexPath.row
    if showCitymobilBanner && (index > SearchCategoriesViewController.citymobilIndex) {
      return categories[index - 1]
    } else {
      return categories[index]
    }
  }
  
  func openBanner() {
    UIApplication.shared.open(bannerUrl!)
    Statistics.logEvent(kStatSearchSponsoredSelect);
  }
}

extension SearchCategoriesViewController: SearchBannerCellDelegate {
  func cellDidPressAction(_ cell: SearchBannerCell) {
    openBanner()
  }
  
  func cellDidPressClose(_ cell: SearchBannerCell) {
    MapViewController.shared().showRemoveAds()
  }
}
