protocol SearchCategoriesViewControllerDelegate: AnyObject {
  func categoriesViewController(_ viewController: SearchCategoriesViewController,
                                didSelect category: String)
}

final class SearchCategoriesViewController: MWMTableViewController {
  private weak var delegate: SearchCategoriesViewControllerDelegate?
  private let categories: [String]
  private let showMegafonBanner: Bool
  private let bannerUrl: URL
  private static let megafonIndex = 6
  
  init(frameworkHelper: MWMSearchFrameworkHelper, delegate: SearchCategoriesViewControllerDelegate?) {
    self.delegate = delegate
    categories = frameworkHelper.searchCategories()
    showMegafonBanner = frameworkHelper.hasMegafonCategoryBanner()
    bannerUrl = frameworkHelper.megafonBannerUrl()

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
    return categories.count + (showMegafonBanner ? 1 : 0)
  }
  
  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    if showMegafonBanner && (indexPath.row == SearchCategoriesViewController.megafonIndex) {
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
    if showMegafonBanner && (index > SearchCategoriesViewController.megafonIndex) {
      return categories[index - 1]
    } else {
      return categories[index]
    }
  }
}

extension SearchCategoriesViewController: SearchBannerCellDelegate {
  func cellDidPressAction(_ cell: SearchBannerCell) {
    UIApplication.shared.open(bannerUrl)
  }
  
  func cellDidPressClose(_ cell: SearchBannerCell) {
    MapViewController.shared().showRemoveAds()
  }
}
