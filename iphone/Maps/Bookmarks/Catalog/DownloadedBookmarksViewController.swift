class DownloadedBookmarksViewController: MWMViewController {

  @IBOutlet var bottomView: UIView!
  @IBOutlet weak var noDataView: UIView!
  @IBOutlet weak var tableView: UITableView!
  @IBOutlet weak var bottomViewTitleLabel: UILabel! {
    didSet {
      bottomViewTitleLabel.text = L("guides_catalogue_title").uppercased()
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
  private weak var coordinator: BookmarksCoordinator?

  init(coordinator: BookmarksCoordinator?) {
    super.init(nibName: nil, bundle: nil)
    self.coordinator = coordinator
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    tableView.tableHeaderView = bottomView
    tableView.registerNib(cell: CatalogCategoryCell.self)
    tableView.registerNibForHeaderFooterView(BMCCategoriesHeader.self)
    checkInvalidSubscription { [weak self] deleted in
      if deleted {
        self?.reloadData()
      }
    }
    if #available(iOS 11, *) { return } // workaround for https://jira.mail.ru/browse/MAPSME-8101
    reloadData()
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    reloadData()
    
    Statistics.logEvent(kStatGuidesShown, withParameters: [kStatServerIds : dataSource.guideIds],
                        with: .realtime)
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
    Statistics.logEvent(kStatCatalogOpen, withParameters: [kStatFrom: kStatDownloaded])
    let webViewController = CatalogWebViewController.catalogFromAbsoluteUrl(nil, utm: .bookmarksPageCatalogButton)
    MapViewController.topViewController().navigationController?.pushViewController(webViewController, animated: true)
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
    Statistics.logEvent(kStatBookmarkVisibilityChange, withParameters: [kStatFrom : kStatBookmarkList,
                                                                        kStatAction : visible ? kStatShow : kStatHide])
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

  private func openCategory(category: BookmarkGroup) {
    let bmViewController = BookmarksListBuilder.build(markGroupId: category.categoryId, bookmarksCoordinator: coordinator)
    MapViewController.topViewController().navigationController?.pushViewController(bmViewController,
                                                                                   animated: true)
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
    
    if (dataSource.isGuide(at: indexPath.row)) {
      Statistics.logEvent(kStatGuidesOpen, withParameters: [kStatServerId : dataSource.getServerId(at: indexPath.row)],
                          with: .realtime)
    }
    
    let category = dataSource.category(at: indexPath.row)
    openCategory(category: category)
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
