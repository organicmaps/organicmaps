@objc(MWMBookmarksTabViewController)
final class BookmarksTabViewController: TabViewController {
  @objc enum ActiveTab: Int {
    case user = 0
    case catalog
  }

  private static let selectedIndexKey = "BookmarksTabViewController_selectedIndexKey"

  @objc public var activeTab: ActiveTab = ActiveTab(rawValue:
    UserDefaults.standard.integer(forKey: BookmarksTabViewController.selectedIndexKey)) ?? .user {
    didSet {
      UserDefaults.standard.set(activeTab.rawValue, forKey: BookmarksTabViewController.selectedIndexKey)
    }
  }

  private weak var coordinator: BookmarksCoordinator?

  @objc init(coordinator: BookmarksCoordinator?) {
    super.init(nibName: nil, bundle: nil)
    self.coordinator = coordinator
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    let bookmarks = BMCViewController(coordinator: coordinator)
    let catalog = DownloadedBookmarksViewController(coordinator: coordinator)
    bookmarks.title = L("bookmarks")
    catalog.title = L("guides")
    viewControllers = [bookmarks, catalog]

    title = L("bookmarks_guides");
    tabView.selectedIndex = activeTab.rawValue
    tabView.delegate = self
  }

  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    activeTab = ActiveTab(rawValue: tabView.selectedIndex ?? 0) ?? .user
  }
}

extension BookmarksTabViewController: TabViewDelegate {
  func tabView(_ tabView: TabView, didSelectTabAt index: Int) {
    let selectedTab = index == 0 ? "my" : "downloaded"
    Statistics.logEvent("Bookmarks_Tab_click", withParameters: [kStatValue : selectedTab])
  }
}
