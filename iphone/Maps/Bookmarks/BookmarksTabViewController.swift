@objc(MWMBookmarksTabViewController)
final class BookmarksTabViewController: TabViewController {
  private static let selectedIndexKey = "BookmarksTabViewController_selectedIndexKey"
  private var selectedIndex: Int {
    get {
      return UserDefaults.standard.integer(forKey: BookmarksTabViewController.selectedIndexKey)
    }
    set {
      UserDefaults.standard.set(newValue, forKey: BookmarksTabViewController.selectedIndexKey)
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
    tabView.selectedIndex = selectedIndex
  }

  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    selectedIndex = tabView.selectedIndex
  }
}
