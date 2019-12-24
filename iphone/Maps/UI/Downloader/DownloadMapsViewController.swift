@objc(MWMDownloadMapsViewController)
class DownloadMapsViewController: MWMViewController {
  // MARK: - Types

  private enum NodeAction {
    case showOnMap
    case download
    case update
    case cancelDownload
    case retryDownload
    case delete
  }

  private enum AllMapsButtonState {
    case none
    case download(String)
    case cancel(String)
  }

  // MARK: - Outlets

  @IBOutlet var tableView: UITableView!
  @IBOutlet var allMapsView: UIView!
  @IBOutlet var allMapsViewBottomOffset: NSLayoutConstraint!
  @IBOutlet var allMapsButton: UIButton!
  @IBOutlet var allMapsCancelButton: UIButton!
  @IBOutlet var searchBar: UISearchBar!
  @IBOutlet var statusBarBackground: UIView!
  @IBOutlet var noMapsContainer: UIView!

  // MARK: - Properties

  var dataSource: IDownloaderDataSource!
  @objc var mode: MWMMapDownloaderMode = .downloaded
  private var skipCountryEvent = false

  lazy var noSerchResultViewController: SearchNoResultsViewController = {
    let vc = storyboard!.instantiateViewController(ofType: SearchNoResultsViewController.self)
    view.insertSubview(vc.view, belowSubview: statusBarBackground)
    vc.view.alignToSuperview()
    vc.view.isHidden = true
    addChild(vc)
    vc.didMove(toParent: self)
    return vc
  }()

  // MARK: - Methods

  override func viewDidLoad() {
    super.viewDidLoad()
    if dataSource == nil {
      switch mode {
      case .downloaded:
        dataSource = DownloadedMapsDataSource()
      case .available:
        dataSource = AvailableMapsDataSource(location: MWMLocationManager.lastLocation()?.coordinate)
      @unknown default:
        fatalError()
      }
    }
    tableView.registerNib(cell: MWMMapDownloaderTableViewCell.self)
    tableView.registerNib(cell: MWMMapDownloaderPlaceTableViewCell.self)
    tableView.registerNib(cell: MWMMapDownloaderLargeCountryTableViewCell.self)
    tableView.registerNib(cell: MWMMapDownloaderSubplaceTableViewCell.self)
    title = dataSource.title
    if mode == .downloaded {
      let addMapsButton = button(with: UIImage(named: "ic_nav_bar_add"), action: #selector(onAddMaps))
      navigationItem.rightBarButtonItem = addMapsButton
    }
    MWMFrameworkListener.add(self)
    noMapsContainer.isHidden = !dataSource.isEmpty || Storage.downloadInProgress()
    configButtons()
  }

  fileprivate func showChildren(_ nodeAttrs: MapNodeAttributes) {
    let vc = storyboard!.instantiateViewController(ofType: DownloadMapsViewController.self)
    vc.mode = mode
    vc.dataSource = dataSource.dataSourceFor(nodeAttrs.countryId)
    navigationController?.pushViewController(vc, animated: true)
  }

  fileprivate func showActions(_ nodeAttrs: MapNodeAttributes) {
    let menuTitle = nodeAttrs.nodeName
    let multiparent = nodeAttrs.parentInfo.count > 1
    let message = dataSource.isRoot || multiparent ? nil : nodeAttrs.parentInfo.first?.countryName
    let actionSheet = UIAlertController(title: menuTitle, message: message, preferredStyle: .actionSheet)

    let actions: [NodeAction]
    switch nodeAttrs.nodeStatus {
    case .undefined:
      actions = []
    case .downloading, .applying, .inQueue:
      actions = [.cancelDownload]
    case .error:
      actions = nodeAttrs.downloadedMwmCount > 0 ? [.retryDownload, .delete] : [.retryDownload]
    case .onDiskOutOfDate:
      actions = [.showOnMap, .update, .delete]
    case .onDisk:
      actions = [.showOnMap, .delete]
    case .notDownloaded:
      actions = [.download]
    case .partly:
      actions = [.download, .delete]
    @unknown default:
      fatalError()
    }

    addActions(actions, for: nodeAttrs, to: actionSheet)
    actionSheet.addAction(UIAlertAction(title: L("cancel"), style: .cancel))
    present(actionSheet, animated: true)
  }

  private func addActions(_ actions: [NodeAction], for nodeAttrs: MapNodeAttributes, to actionSheet: UIAlertController) {
    actions.forEach { [unowned self] in
      let action: UIAlertAction
      switch $0 {
      case .showOnMap:
        action = UIAlertAction(title: L("zoom_to_country"), style: .default, handler: { _ in
          Storage.showNode(nodeAttrs.countryId)
          self.navigationController?.popToRootViewController(animated: true)
        })
      case .download:
        let prefix = nodeAttrs.totalMwmCount == 1 ? L("downloader_download_map") : L("downloader_download_all_button")
        action = UIAlertAction(title: "\(prefix) (\(formattedSize(nodeAttrs.totalSize)))",
                               style: .default,
                               handler: { _ in
                                Storage.downloadNode(nodeAttrs.countryId)
        })
      case .update:
        let title = "\(L("downloader_status_outdated")) (TODO: updated size)"
        action = UIAlertAction(title: title, style: .default, handler: { _ in
          Storage.updateNode(nodeAttrs.countryId)
        })
      case .cancelDownload:
        action = UIAlertAction(title: L("cancel_download"), style: .destructive, handler: { _ in
          Storage.cancelDownloadNode(nodeAttrs.countryId)
        })
      case .retryDownload:
        action = UIAlertAction(title: L("downloader_retry"), style: .destructive, handler: { _ in
          Storage.retryDownloadNode(nodeAttrs.countryId)
        })
      case .delete:
        action = UIAlertAction(title: L("downloader_delete_map"), style: .destructive, handler: { _ in
          Storage.deleteNode(nodeAttrs.countryId)
        })
      }
      actionSheet.addAction(action)
    }
  }

  private func setAllMapsButton(_ state: AllMapsButtonState) {
    switch state {
    case .none:
      allMapsView.isHidden = true
    case .download(let buttonTitle):
      allMapsView.isHidden = false
      allMapsButton.isHidden = false
      allMapsButton.setTitle(buttonTitle, for: .normal)
      allMapsCancelButton.isHidden = true
    case .cancel(let buttonTitle):
      allMapsView.isHidden = false
      allMapsButton.isHidden = true
      allMapsCancelButton.isHidden = false
      allMapsCancelButton.setTitle(buttonTitle, for: .normal)
    }
  }

  fileprivate func configButtons() {
    allMapsView.isHidden = true
    if mode == .available {
      if dataSource.isRoot {
        setAllMapsButton(.none)
      } else {
        let parentAttributes = dataSource.parentAttributes()
        if parentAttributes.downloadingMwmCount > 0 {
          setAllMapsButton(.cancel("\(L("downloader_cancel_all")) (\(formattedSize(parentAttributes.downloadingSize)))"))
        } else if parentAttributes.downloadedMwmCount < parentAttributes.totalMwmCount {
          setAllMapsButton(.download("\(L("downloader_download_all_button")) (\(formattedSize(parentAttributes.totalSize - parentAttributes.downloadedSize)))"))
        }
      }
    } else {
      let updateInfo = Storage.updateInfo(withParent: dataSource.parentAttributes().countryId)
      if updateInfo.numberOfFiles > 0 {
        setAllMapsButton(.download("\(L("downloader_update_all_button")) (\(formattedSize(updateInfo.updateSize)))"))
      } else {
        let parentAttributes = dataSource.parentAttributes()
        if parentAttributes.downloadingMwmCount > 0 {
          setAllMapsButton(.cancel(L("downloader_cancel_all")))
        }
      }
    }
  }

  @objc func onAddMaps() {
    let vc = storyboard!.instantiateViewController(ofType: DownloadMapsViewController.self)
    vc.mode = .available
    navigationController?.pushViewController(vc, animated: true)
  }

  @IBAction func onAllMaps(_ sender: UIButton) {
    skipCountryEvent = true
    if mode == .downloaded {
      Storage.updateNode(dataSource.parentAttributes().countryId)
    } else {
      Storage.downloadNode(dataSource.parentAttributes().countryId)
    }
    skipCountryEvent = false
    self.processCountryEvent(dataSource.parentAttributes().countryId)
  }

  @IBAction func onCancelAllMaps(_ sender: UIButton) {
    skipCountryEvent = true
    Storage.cancelDownloadNode(dataSource.parentAttributes().countryId)
    skipCountryEvent = false
    self.processCountryEvent(dataSource.parentAttributes().countryId)
    tableView.reloadData()
  }
}

// MARK: - UITableViewDataSource

extension DownloadMapsViewController: UITableViewDataSource {
  func numberOfSections(in tableView: UITableView) -> Int {
    dataSource.numberOfSections()
  }
  
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    dataSource.numberOfItems(in: section)
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let nodeAttrs = dataSource.item(at: indexPath)
    let cell: MWMMapDownloaderTableViewCell
    if dataSource.item(at: indexPath).hasChildren {
      let cellType = MWMMapDownloaderLargeCountryTableViewCell.self
      let largeCountryCell = tableView.dequeueReusableCell(cell: cellType, indexPath: indexPath)
      cell = largeCountryCell
    } else if let matchedName = dataSource.matchedName(at: indexPath), matchedName != nodeAttrs.nodeName {
      let cellType = MWMMapDownloaderSubplaceTableViewCell.self
      let subplaceCell = tableView.dequeueReusableCell(cell: cellType, indexPath: indexPath)
      subplaceCell.setSubplaceText(matchedName)
      cell = subplaceCell
    } else if !nodeAttrs.hasParent {
      let cellType = MWMMapDownloaderTableViewCell.self
      let downloaderCell = tableView.dequeueReusableCell(cell: cellType, indexPath: indexPath)
      cell = downloaderCell
    } else {
      let cellType = MWMMapDownloaderPlaceTableViewCell.self
      let placeCell = tableView.dequeueReusableCell(cell: cellType, indexPath: indexPath)
      cell = placeCell
    }
    cell.mode = dataSource.isSearching ? .available : mode
    cell.config(nodeAttrs, searchQuery: searchBar.text)
    cell.delegate = self
    return cell
  }

  func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
    dataSource.title(for: section)
  }

  func sectionIndexTitles(for tableView: UITableView) -> [String]? {
    dataSource.indexTitles()
  }

  func tableView(_ tableView: UITableView, sectionForSectionIndexTitle title: String, at index: Int) -> Int {
    index
  }

  func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    let nodeAttrs = dataSource.item(at: indexPath)
    switch nodeAttrs.nodeStatus {
    case .onDisk, .onDiskOutOfDate, .partly:
      return true
    default:
      return false
    }
  }

  func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
    if editingStyle == .delete{
      let nodeAttrs = dataSource.item(at: indexPath)
      Storage.deleteNode(nodeAttrs.countryId)
    }
  }
}

// MARK: - UITableViewDelegate

extension DownloadMapsViewController: UITableViewDelegate {
  func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    let headerView = MWMMapDownloaderCellHeader()
    headerView.text = dataSource.title(for: section)
    return headerView
  }

  func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    28
  }

  func tableView(_ tableView: UITableView, heightForFooterInSection section: Int) -> CGFloat {
    section == dataSource.numberOfSections() - 1 ? 68 : 0
  }

  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    let nodeAttrs = dataSource.item(at: indexPath)
    if nodeAttrs.hasChildren {
      showChildren(dataSource.item(at: indexPath))
      return
    }
    showActions(nodeAttrs)
  }
}

// MARK: - UIScrollViewDelegate

extension DownloadMapsViewController: UIScrollViewDelegate {
  func scrollViewWillBeginDragging(_ scrollView: UIScrollView) {
    searchBar.resignFirstResponder()
  }
}

// MARK: - MWMMapDownloaderTableViewCellDelegate

extension DownloadMapsViewController: MWMMapDownloaderTableViewCellDelegate {
  func mapDownloaderCellDidPressProgress(_ cell: MWMMapDownloaderTableViewCell) {
    guard let indexPath = tableView.indexPath(for: cell) else { return }
    let nodeAttrs = dataSource.item(at: indexPath)
    switch (nodeAttrs.nodeStatus) {
    case .undefined, .error:
      Storage.retryDownloadNode(nodeAttrs.countryId)
      break
    case .downloading, .applying, .inQueue:
      Storage.cancelDownloadNode(nodeAttrs.countryId)
      break
    case .onDiskOutOfDate:
      Storage.updateNode(nodeAttrs.countryId)
      break
    case .onDisk:
      //do nothing
      break
    case .notDownloaded, .partly:
      if nodeAttrs.hasChildren {
        showChildren(nodeAttrs)
      } else {
        Storage.downloadNode(nodeAttrs.countryId)
      }
      break
    @unknown default:
      fatalError()
    }
  }

  func mapDownloaderCellDidLongPress(_ cell: MWMMapDownloaderTableViewCell) {
    guard let indexPath = tableView.indexPath(for: cell) else { return }
    let nodeAttrs = dataSource.item(at: indexPath)
    showActions(nodeAttrs)
  }
}

// MARK: - MWMFrameworkStorageObserver

extension DownloadMapsViewController: MWMFrameworkStorageObserver {
  func processCountryEvent(_ countryId: String) {
    print("processCountryEvent: \(countryId)")
    if skipCountryEvent && countryId == dataSource.parentAttributes().countryId {
      return
    }
    dataSource.reload {
      tableView.reloadData()
      noMapsContainer.isHidden = !dataSource.isEmpty || Storage.downloadInProgress()
    }
    if countryId == dataSource.parentAttributes().countryId {
      configButtons()
    }

    for cell in tableView.visibleCells {
      guard let downloaderCell = cell as? MWMMapDownloaderTableViewCell else { continue }
      if downloaderCell.nodeAttrs.countryId != countryId { continue }
      guard let indexPath = tableView.indexPath(for: downloaderCell) else { return }
      downloaderCell.config(dataSource.item(at: indexPath), searchQuery: searchBar.text)
    }
  }

  func processCountry(_ countryId: String, downloadedBytes: UInt64, totalBytes: UInt64) {
    for cell in tableView.visibleCells {
      guard let downloaderCell = cell as? MWMMapDownloaderTableViewCell else { continue }
      if downloaderCell.nodeAttrs.countryId != countryId { continue }
      downloaderCell.setDownloadProgress(CGFloat(downloadedBytes) / CGFloat(totalBytes))
    }
  }
}

// MARK: - UISearchBarDelegate

extension DownloadMapsViewController: UISearchBarDelegate {
  func searchBarShouldBeginEditing(_ searchBar: UISearchBar) -> Bool {
    searchBar.setShowsCancelButton(true, animated: true)
    navigationController?.setNavigationBarHidden(true, animated: true)
    tableView.contentInset = .zero
    tableView.scrollIndicatorInsets = .zero
    return true
  }

  func searchBarShouldEndEditing(_ searchBar: UISearchBar) -> Bool {
    searchBar.setShowsCancelButton(false, animated: true)
    navigationController?.setNavigationBarHidden(false, animated: true)
    tableView.contentInset = .zero
    tableView.scrollIndicatorInsets = .zero
    return true
  }

  func searchBarCancelButtonClicked(_ searchBar: UISearchBar) {
    searchBar.text = nil
    searchBar.resignFirstResponder()
    dataSource.cancelSearch()
    tableView.reloadData()
    self.noSerchResultViewController.view.isHidden = true
  }

  func searchBar(_ searchBar: UISearchBar, textDidChange searchText: String) {
    let locale = searchBar.textInputMode?.primaryLanguage
    dataSource.search(searchText, locale: locale ?? "") { [weak self] (finished) in
      guard let self = self else { return }
      self.tableView.reloadData()
      self.noSerchResultViewController.view.isHidden = !self.dataSource.isEmpty
    }
  }
}

// MARK: - UIBarPositioningDelegate

extension DownloadMapsViewController: UIBarPositioningDelegate {
  func position(for bar: UIBarPositioning) -> UIBarPosition {
    .topAttached
  }
}
