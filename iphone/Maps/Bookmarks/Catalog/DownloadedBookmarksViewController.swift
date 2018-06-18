class DownloadedBookmarksViewController: UITableViewController {

  @IBOutlet var topView: UIView!
  @IBOutlet var bottomView: UIView!

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

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    dataSource.reload()
    tableView.reloadData()
  }

  override func numberOfSections(in tableView: UITableView) -> Int {
    return dataSource.categoriesCount > 0 ? 1 : 0
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return dataSource.categoriesCount
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(cell: CatalogCategoryCell.self, indexPath: indexPath)
    cell.update(with: dataSource.category(at: indexPath.row), delegate: self)
    return cell
  }

  override func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    return 48
  }

  override func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    let headerView = tableView.dequeueReusableHeaderFooterView(BMCCategoriesHeader.self)
    headerView.isShowAll = !dataSource.allCategoriesVisible
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

  private func setCategoryVisible(_ visible: Bool, at index: Int) {
    dataSource.setCategory(visible: visible, at: index)
    let categoriesHeader = tableView.headerView(forSection: 0) as! BMCCategoriesHeader
    categoriesHeader.isShowAll = !dataSource.allCategoriesVisible
  }

  private func shareCategory(at index: Int) {
    let category = dataSource.category(at: index)
    if let url = MWMBookmarksManager.sharingUrl(forCategoryId: category.categoryId) {
      let message = L("share_bookmarks_email_body")
      let shareController = MWMActivityViewController.share(for: url, message: message)
      shareController?.present(inParentViewController: self, anchorView: nil)
    }
  }

  private func deleteCategory(at index: Int) {
    self.dataSource.deleteCategory(at: index)
    if self.dataSource.categoriesCount > 0 {
      self.tableView.deleteRows(at: [IndexPath(row: index, section: 0)], with: .automatic)
    } else {
      self.tableView.tableHeaderView = self.topView
      self.tableView.reloadData()
    }
  }
}

extension DownloadedBookmarksViewController: CatalogCategoryCellDelegate {
  func cell(_ cell: CatalogCategoryCell, didCheck visible: Bool) {
    if let indexPath = tableView.indexPath(for: cell) {
      setCategoryVisible(visible, at: indexPath.row)
    }
  }

  func cell(_ cell: CatalogCategoryCell, didPress moreButton: UIButton) {
    if let indexPath = tableView.indexPath(for: cell) {
      let category = dataSource.category(at: indexPath.row)
      let actionSheet = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
      if let ppc = actionSheet.popoverPresentationController {
        ppc.sourceView = moreButton
        ppc.sourceRect = moreButton.bounds
      }

      let showHide = L(category.isVisible ? "hide" : "show").capitalized
      actionSheet.addAction(UIAlertAction(title: showHide, style: .default, handler: { _ in
        self.setCategoryVisible(!category.isVisible, at: indexPath.row)
        self.tableView.reloadRows(at: [indexPath], with: .none)
      }))

      let share = L("share").capitalized
      actionSheet.addAction(UIAlertAction(title: share, style: .default, handler: { _ in
        self.shareCategory(at: indexPath.row)
      }))

      let delete = L("delete").capitalized
      let deleteAction = UIAlertAction(title: delete, style: .destructive, handler: { _ in
        self.deleteCategory(at: indexPath.row)
      })
      actionSheet.addAction(deleteAction)
      let cancel = L("cancel").capitalized
      actionSheet.addAction(UIAlertAction(title: cancel, style: .cancel, handler: nil))

      present(actionSheet, animated: true, completion: nil)
    }
  }
}

extension DownloadedBookmarksViewController: BMCCategoriesHeaderDelegate {
  func visibilityAction(_ categoriesHeader: BMCCategoriesHeader) {
    let showAll = categoriesHeader.isShowAll
    dataSource.allCategoriesVisible = showAll
    tableView.reloadData()
  }
}
