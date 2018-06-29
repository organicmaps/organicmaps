@objc(MWMBookmarksTabViewController)
final class BookmarksTabViewController: TabViewController {
  @objc enum ActiveTab: Int {
    case user = 0
    case catalog
  }

  private static let selectedIndexKey = "BookmarksTabViewController_selectedIndexKey"

  @objc public var activeTab: ActiveTab = ActiveTab.init(rawValue:
    UserDefaults.standard.integer(forKey: BookmarksTabViewController.selectedIndexKey)) ?? .user {
    didSet {
      UserDefaults.standard.set(activeTab.rawValue, forKey: BookmarksTabViewController.selectedIndexKey)
    }
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    let bookmarks = BMCViewController()
    let catalog = DownloadedBookmarksViewController()
    bookmarks.title = L("bookmarks_page_my")
    catalog.title = L("bookmarks_page_downloaded")
    viewControllers = [bookmarks, catalog]

    title = L("bookmarks");
    tabView.barTintColor = .primary()
    tabView.tintColor = .white()
    tabView.headerTextAttributes = [.foregroundColor: UIColor.whitePrimaryText(),
                                    .font: UIFont.medium14()]
    tabView.selectedIndex = activeTab.rawValue
  }

  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    activeTab = ActiveTab.init(rawValue: tabView.selectedIndex) ?? .user
  }
}
