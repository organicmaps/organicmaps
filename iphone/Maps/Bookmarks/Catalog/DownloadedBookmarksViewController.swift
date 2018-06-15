class DownloadedBookmarksViewController: UITableViewController {

  @IBOutlet weak var topView: UIView!
  @IBOutlet weak var bottomView: UIView!

  override func viewDidLoad() {
    super.viewDidLoad()

    tableView.tableHeaderView = topView
    tableView.tableFooterView = bottomView
  }

  override func numberOfSections(in tableView: UITableView) -> Int {
    return 0
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return 0
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(withIdentifier: "reuseIdentifier", for: indexPath)

    return cell
  }


  @IBAction func onDownloadBookmarks(_ sender: Any) {
    if let url = MWMBookmarksManager.catalogFrontendUrl(),
      let webViewController = CatalogWebViewController(url: url, andTitleOrNil: L("routes_and_bookmarks")) {
      MapViewController.topViewController().navigationController?.pushViewController(webViewController,
                                                                                     animated: true)
    }
  }
}
