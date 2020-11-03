protocol SearchCategoriesViewControllerDelegate: AnyObject {
  func categoriesViewController(_ viewController: SearchCategoriesViewController,
                                didSelect category: String)
}

final class SearchCategoriesViewController: MWMTableViewController {
  private weak var delegate: SearchCategoriesViewControllerDelegate?
  private let categories: [String]
  private let banner: MWMBanner?
  private var bannerShown = false
  private static let bannerIndex = 6
  
  init(frameworkHelper: MWMSearchFrameworkHelper, delegate: SearchCategoriesViewControllerDelegate?) {
    self.delegate = delegate
    categories = frameworkHelper.searchCategories()
    banner = frameworkHelper.searchCategoryBanner()
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
    return categories.count + (banner != nil ? 1 : 0)
  }
  
  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    if banner != nil && (indexPath.row == SearchCategoriesViewController.bannerIndex) {
      return createBanner(indexPath)
    }
    
    let cell = tableView.dequeueReusableCell(cell: SearchCategoryCell.self, indexPath: indexPath)
    cell.update(with: category(at: indexPath))
    return cell
  }
  
  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    if banner != nil && (indexPath.row == SearchCategoriesViewController.bannerIndex) {
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
    if banner != nil && (index > SearchCategoriesViewController.bannerIndex) {
      return categories[index - 1]
    } else {
      return categories[index]
    }
  }
  
  private func bannerStatProvider(_ bannerType: MWMBannerType) -> String {
    switch bannerType {
      case .citymobil: return kStatCitymobil
      default: return ""
    }
  }
  
  private func createBanner(_ indexPath: IndexPath) -> UITableViewCell {
    guard let banner = banner else { fatalError("Banner must exist") }
    let cell = tableView.dequeueReusableCell(cell: SearchBannerCell.self, indexPath: indexPath)
    switch banner.mwmType {
      case .citymobil:
        cell.configure(icon: "ic_taxi_logo_citymobil",
                       label: L("taxi"),
                       buttonText: L("taxi_category_order"),
                       delegate: self)
      default: fatalError("Unexpected banner type")
    }
    if (!bannerShown) {
      bannerShown = true;
      let provider = bannerStatProvider(banner.mwmType)
      Statistics.logEvent(kStatSearchSponsoredShow, withParameters: [kStatProvider: provider]);
    }
    return cell

  }
  
  private func openBanner() {
    guard let banner = banner else { fatalError("Banner must exist") }
    if let url = URL(string: banner.bannerID) {
      UIApplication.shared.open(url)
    }
    let provider = bannerStatProvider(banner.mwmType)
    Statistics.logEvent(kStatSearchSponsoredSelect, withParameters: [kStatProvider: provider]);
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
