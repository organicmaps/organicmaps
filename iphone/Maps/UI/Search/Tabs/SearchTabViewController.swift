@objc
protocol SearchTabViewControllerDelegate: UIScrollViewDelegate {
  func searchTabController(_ viewController: SearchTabViewController, didSearch: SearchQuery)
}

@objc
final class SearchTabViewController: TabViewController {
  private enum Tab: Int {
    case history
    case categories
  }

  private static let selectedIndexKey = "SearchTabViewController_selectedIndexKey"
  @objc weak var delegate: SearchTabViewControllerDelegate?

  private var frameworkHelper = MWMSearchFrameworkHelper.self
  private var tabs: [Tab] = []

  private var activeTab: Tab = .init(rawValue:
    UserDefaults.standard.integer(forKey: SearchTabViewController.selectedIndexKey)) ?? .categories {
    didSet {
      UserDefaults.standard.set(activeTab.rawValue, forKey: SearchTabViewController.selectedIndexKey)
    }
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    tabs = Settings.searchHistoryEnabled() ? [.history, .categories] : [.categories]
    viewControllers = tabs.map { viewController(for: $0) }
    tabView.showsTabBar = tabs.count > 1

    let selectedTab: Tab
    if !tabs.contains(.history) || frameworkHelper.isSearchHistoryEmpty() {
      selectedTab = .categories
    } else {
      selectedTab = activeTab
    }
    tabView.selectedIndex = tabs.firstIndex(of: selectedTab) ?? 0
  }

  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    guard let selectedIndex = tabView.selectedIndex,
          tabs.indices.contains(selectedIndex) else { return }
    activeTab = tabs[selectedIndex]
  }

  func reloadSearchHistory() {
    guard let historyIndex = tabs.firstIndex(of: .history) else { return }
    (viewControllers[historyIndex] as? SearchHistoryViewController)?.reload()
  }

  private func viewController(for tab: Tab) -> UIViewController {
    switch tab {
    case .history:
      let history = SearchHistoryViewController(frameworkHelper: frameworkHelper,
                                                delegate: self)
      history.title = L("history")
      return history
    case .categories:
      let categories = SearchCategoriesViewController(frameworkHelper: frameworkHelper,
                                                      delegate: self)
      categories.title = L("categories")
      return categories
    }
  }
}

extension SearchTabViewController: ModallyPresentedViewController {
  func presentationFrameDidChange(_ frame: CGRect) {
    viewControllers.forEach { ($0 as? ModallyPresentedViewController)?.presentationFrameDidChange(frame) }
  }
}

extension SearchTabViewController: UIScrollViewDelegate {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    delegate?.scrollViewDidScroll?(scrollView)
  }

  func scrollViewWillEndDragging(_ scrollView: UIScrollView, withVelocity velocity: CGPoint, targetContentOffset: UnsafeMutablePointer<CGPoint>) {
    delegate?.scrollViewWillEndDragging?(scrollView, withVelocity: velocity, targetContentOffset: targetContentOffset)
  }
}

extension SearchTabViewController: SearchCategoriesViewControllerDelegate {
  func categoriesViewController(_: SearchCategoriesViewController,
                                didSelect category: String) {
    let preferredLang = AppInfo.shared().languageId
    let supportedBySearchLang = MWMSearchFrameworkHelper.isLanguageSupported(preferredLang) ? preferredLang : "en"
    let searchText = L(category, languageCode: supportedBySearchLang) + " "
    let query = SearchQuery(searchText, locale: supportedBySearchLang, source: .category)
    delegate?.searchTabController(self, didSearch: query)
  }
}

extension SearchTabViewController: SearchHistoryViewControllerDelegate {
  func searchHistoryViewController(_: SearchHistoryViewController,
                                   didSelect query: String) {
    let query = SearchQuery(query.trimmingCharacters(in: .whitespacesAndNewlines) + " ", source: .history)
    delegate?.searchTabController(self, didSearch: query)
  }
}
