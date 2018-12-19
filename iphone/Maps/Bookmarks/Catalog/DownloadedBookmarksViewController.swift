class DownloadedBookmarksViewController: MWMViewController {

  @IBOutlet var bottomView: UIView!
  @IBOutlet weak var noDataView: UIView!
  @IBOutlet weak var tableView: UITableView!
  @IBOutlet weak var bottomViewTitleLabel: UILabel! {
    didSet {
      bottomViewTitleLabel.text = L("download_guides").uppercased()
    }
  }

  @IBOutlet weak var bottomViewDownloadButton: UIButton! {
    didSet {
      bottomViewDownloadButton.setTitle(L("download_guides").uppercased(), for: .normal)
    }
  }

  @IBOutlet weak var noDataViewDownloadButton: UIButton! {
    didSet {
      noDataViewDownloadButton.setTitle(L("download_guides").uppercased(), for: .normal)
    }
  }

  let dataSource = DownloadedBookmarksDataSource()

  override func viewDidLoad() {
    super.viewDidLoad()
    tableView.backgroundColor = UIColor.pressBackground()
    tableView.separatorColor = UIColor.blackDividers()
    tableView.tableHeaderView = bottomView
    tableView.registerNib(cell: CatalogCategoryCell.self)
    tableView.registerNibForHeaderFooterView(BMCCategoriesHeader.self)
    if #available(iOS 11, *) { return } // workaround for https://jira.mail.ru/browse/MAPSME-8101
    reloadData()
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    reloadData()
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    var f = bottomView.frame
    let s = bottomView.systemLayoutSizeFitting(CGSize(width: tableView.width, height: 1),
                                               withHorizontalFittingPriority: .required,
                                               verticalFittingPriority: .defaultLow)
    f.size = s
    bottomView.frame = f
    tableView.refresh()
  }

  @IBAction func onDownloadBookmarks(_ sender: Any) {
    if MWMPlatform.networkConnectionType() == .none {
      MWMAlertViewController.activeAlert().presentNoConnectionAlert();
      Statistics.logEvent("Bookmarks_Downloaded_Catalogue_error",
                          withParameters: [kStatError : "no_internet"])
      return
    }
    Statistics.logEvent("Bookmarks_Downloaded_Catalogue_open")
    let webViewController = CatalogWebViewController()
    MapViewController.topViewController().navigationController?.pushViewController(webViewController,
                                                                                   animated: true)
  }

  private func reloadData() {
    dataSource.reloadData()
    noDataView.isHidden = dataSource.categoriesCount > 0
    tableView.reloadData()
  }

  private func setCategoryVisible(_ visible: Bool, at index: Int) {
    dataSource.setCategory(visible: visible, at: index)
    if let categoriesHeader = tableView.headerView(forSection: 0) as? BMCCategoriesHeader {
      categoriesHeader.isShowAll = dataSource.allCategoriesHidden
    }
  }

  private func deleteCategory(at index: Int) {
    guard index >= 0 && index < dataSource.categoriesCount else {
      assertionFailure()
      return
    }
    dataSource.deleteCategory(at: index)
    if dataSource.categoriesCount > 0 {
      tableView.deleteRows(at: [IndexPath(row: index, section: 0)], with: .automatic)
    } else {
      noDataView.isHidden = false
      tableView.reloadData()
    }
  }
}

extension DownloadedBookmarksViewController: UITableViewDataSource {
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return dataSource.categoriesCount
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(cell: CatalogCategoryCell.self, indexPath: indexPath)
    cell.update(with: dataSource.category(at: indexPath.row), delegate: self)
    return cell
  }
}

extension DownloadedBookmarksViewController: UITableViewDelegate {

  func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    return 48
  }

  func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    let headerView = tableView.dequeueReusableHeaderFooterView(BMCCategoriesHeader.self)
    headerView.isShowAll = dataSource.allCategoriesHidden
    headerView.title = L("guides_groups_cached")
    headerView.delegate = self
    return headerView
  }

  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    let category = dataSource.category(at: indexPath.row)
    if let bmViewController = BookmarksVC(category: category.categoryId) {
      MapViewController.topViewController().navigationController?.pushViewController(bmViewController,
                                                                                     animated: true)
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

      let showHide = L(category.visible ? "hide" : "show").capitalized
      actionSheet.addAction(UIAlertAction(title: showHide, style: .default, handler: { _ in
        self.setCategoryVisible(!category.visible, at: indexPath.row)
        self.tableView.reloadRows(at: [indexPath], with: .none)
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
    dataSource.allCategoriesHidden = !dataSource.allCategoriesHidden
    tableView.reloadData()
  }
}
