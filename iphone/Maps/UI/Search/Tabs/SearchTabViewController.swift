@objc(MWMSearchTabViewControllerDelegate)
protocol SearchTabViewControllerDelegate: SearchOnMapScrollViewDelegate {
  func searchTabController(_ viewController: SearchTabViewController, didSearch: SearchQuery)
}

@objc(MWMSearchTabViewController)
final class SearchTabViewController: TabViewController {
  private enum SearchActiveTab: Int {
    case history = 0
    case categories
  }

  private static let selectedIndexKey = "SearchTabViewController_selectedIndexKey"
  @objc weak var delegate: SearchTabViewControllerDelegate?
  
  private var frameworkHelper = MWMSearchFrameworkHelper.self

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
  func presentationFrameDidChange(_ frame: CGRect) {
    viewControllers.forEach { ($0 as? ModallyPresentedViewController)?.presentationFrameDidChange(frame) }
  }
}

extension SearchTabViewController: SearchOnMapScrollViewDelegate {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    delegate?.scrollViewDidScroll(scrollView)
  }

  func scrollViewWillEndDragging(_ scrollView: UIScrollView, withVelocity velocity: CGPoint, targetContentOffset: UnsafeMutablePointer<CGPoint>) {
    delegate?.scrollViewWillEndDragging(scrollView, withVelocity: velocity, targetContentOffset: targetContentOffset)
  }
}

extension SearchTabViewController: SearchCategoriesViewControllerDelegate {
  func categoriesViewController(_ viewController: SearchCategoriesViewController,
                                didSelect category: String) {
    let query = SearchQuery(L(category) + " ", source: .category)
    delegate?.searchTabController(self, didSearch: query)
  }
}

extension SearchTabViewController: SearchHistoryViewControllerDelegate {
  func searchHistoryViewController(_ viewController: SearchHistoryViewController,
                                   didSelect query: String) {
    let query = SearchQuery(query.trimmingCharacters(in: .whitespacesAndNewlines) + " ", source: .history)
    delegate?.searchTabController(self, didSearch: query)
  }
}
