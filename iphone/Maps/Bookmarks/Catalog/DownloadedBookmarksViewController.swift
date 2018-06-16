class DownloadedBookmarksViewController: UITableViewController {

  @IBOutlet weak var topView: UIView!
  @IBOutlet weak var bottomView: UIView!

  let dataSource = DownloadedBookmarksDataSource()

  override func viewDidLoad() {
    super.viewDidLoad()

    if dataSource.categoriesCount == 0 {
      tableView.tableHeaderView = topView
    }
    tableView.tableFooterView = bottomView
    tableView.registerNib(cell: CatalogCategoryCell.self)
    tableView.registerNibForHeaderFooterView(BMCCategoriesHeader.self)
  }

  override func numberOfSections(in tableView: UITableView) -> Int {
    return 1
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return dataSource.categoriesCount
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(cell: CatalogCategoryCell.self, indexPath: indexPath)
    cell.update(with: dataSource.category(at: indexPath.row))
    return cell
  }

  override func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    return 48
  }

  override func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    let headerView = tableView.dequeueReusableHeaderFooterView(BMCCategoriesHeader.self)
    headerView.isShowAll = true
    headerView.delegate = self
    return headerView
  }

  @IBAction func onDownloadBookmarks(_ sender: Any) {
    if let url = MWMBookmarksManager.catalogFrontendUrl(),
      let webViewController = CatalogWebViewController(url: url, andTitleOrNil: L("routes_and_bookmarks")) {
      MapViewController.topViewController().navigationController?.pushViewController(webViewController,
                                                                                     animated: true)
    }
  }
}

extension DownloadedBookmarksViewController: BMCCategoryCellDelegate {
  func visibilityAction(category: BMCCategory) {

  }

  func moreAction(category: BMCCategory, anchor: UIView) {
    
  }
}

extension DownloadedBookmarksViewController: BMCCategoriesHeaderDelegate {
  func visibilityAction(isShowAll: Bool) {

  }
}
