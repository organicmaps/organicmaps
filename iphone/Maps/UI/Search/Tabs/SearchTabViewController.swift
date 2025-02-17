@objc(MWMSearchTabViewControllerDelegate)
protocol SearchTabViewControllerDelegate: SearchOnMapScrollViewDelegate {
  func searchTabController(_ viewController: SearchTabViewController, didSearch: String, withCategory: Bool)
}

@objc(MWMSearchTabViewController)
final class SearchTabViewController: TabViewController {
  private enum SearchActiveTab: Int {
    case history = 0
    case categories
  }
  
  private static let selectedIndexKey = "SearchTabViewController_selectedIndexKey"
  @objc weak var delegate: SearchTabViewControllerDelegate?
  
  private lazy var frameworkHelper: MWMSearchFrameworkHelper = {
    return MWMSearchFrameworkHelper()
  }()
  
  private var activeTab: SearchActiveTab = SearchActiveTab.init(rawValue:
    UserDefaults.standard.integer(forKey: SearchTabViewController.selectedIndexKey)) ?? .categories {
    didSet {
      UserDefaults.standard.set(activeTab.rawValue, forKey: SearchTabViewController.selectedIndexKey)
    }
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    let history = SearchHistoryViewController(frameworkHelper: frameworkHelper,
                                              delegate: self)
    history.title = L("history")
    
    let categories = SearchCategoriesViewController(frameworkHelper: frameworkHelper,
                                                    delegate: self)
    categories.title = L("categories")
    viewControllers = [history, categories]
    
    if frameworkHelper.isSearchHistoryEmpty() {
      tabView.selectedIndex = SearchActiveTab.categories.rawValue
    } else {
      tabView.selectedIndex = activeTab.rawValue
    }
  }
  
  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    activeTab = SearchActiveTab.init(rawValue: tabView.selectedIndex ?? 0) ?? .categories
  }

  func reloadSearchHistory() {
    (viewControllers[SearchActiveTab.history.rawValue] as? SearchHistoryViewController)?.reload()
  }
}

extension SearchTabViewController: ModallyPresentedViewController {
  func translationYDidUpdate(_ translationY: CGFloat) {
    viewControllers.forEach { ($0 as? ModallyPresentedViewController)?.translationYDidUpdate(translationY) }
  }
}

extension SearchTabViewController: SearchOnMapScrollViewDelegate {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    delegate?.scrollViewDidScroll(scrollView)
  }
}

extension SearchTabViewController: SearchCategoriesViewControllerDelegate {
  func categoriesViewController(_ viewController: SearchCategoriesViewController,
                                didSelect category: String) {
    let query = L(category) + " "
    delegate?.searchTabController(self, didSearch: query, withCategory: true)
  }
}

extension SearchTabViewController: SearchHistoryViewControllerDelegate {
  func searchHistoryViewController(_ viewController: SearchHistoryViewController,
                             didSelect query: String) {
    delegate?.searchTabController(self, didSearch: query, withCategory: false)
  }
}
