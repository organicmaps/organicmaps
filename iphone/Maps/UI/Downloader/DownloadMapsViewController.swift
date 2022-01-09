import UIKit

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
  @IBOutlet var noMapsContainer: UIView!
  @IBOutlet var downloadAllViewContainer: UIView!

  // MARK: - Properties

  private var searchBar: UISearchBar = UISearchBar()
  var dataSource: IDownloaderDataSource!
  @objc var mode: MWMMapDownloaderMode = .downloaded
  private var skipCountryEvent = false
  private var hasAddMapSection: Bool { dataSource.isRoot && mode == .downloaded }
  private let allMapsViewBottomOffsetConstant: CGFloat = 64

  lazy var noSerchResultViewController: SearchNoResultsViewController = {
    let vc = storyboard!.instantiateViewController(ofType: SearchNoResultsViewController.self)
    view.insertSubview(vc.view, aboveSubview: tableView)
    vc.view.alignToSuperview()
    vc.view.isHidden = true
    addChild(vc)
    vc.didMove(toParent: self)
    return vc
  }()

  lazy var downloadAllView: DownloadAllView = {
    let view = Bundle.main.load(viewClass: DownloadAllView.self)?.first as! DownloadAllView
    view.delegate = self
    downloadAllViewContainer.addSubview(view)
    view.alignToSuperview()
    return view
  }()

  // MARK: - Methods

  override func viewDidLoad() {
    super.viewDidLoad()
    if dataSource == nil {
      switch mode {
      case .downloaded:
        dataSource = DownloadedMapsDataSource()
      case .available:
        dataSource = AvailableMapsDataSource(location: LocationManager.lastLocation()?.coordinate)
      @unknown default:
        fatalError()
      }
    }
    tableView.registerNib(cell: MWMMapDownloaderTableViewCell.self)
    tableView.registerNib(cell: MWMMapDownloaderPlaceTableViewCell.self)
    tableView.registerNib(cell: MWMMapDownloaderLargeCountryTableViewCell.self)
    tableView.registerNib(cell: MWMMapDownloaderSubplaceTableViewCell.self)
    tableView.registerNib(cell: MWMMapDownloaderButtonTableViewCell.self)
    title = dataSource.title
    if mode == .downloaded {
      let addMapsButton = button(with: UIImage(named: "ic_nav_bar_add"), action: #selector(onAddMaps))
      navigationItem.rightBarButtonItem = addMapsButton
    }
    noMapsContainer.isHidden = !dataSource.isEmpty || Storage.shared().downloadInProgress()

    if dataSource.isRoot {
      searchBar.placeholder = L("downloader_search_field_hint")
      searchBar.delegate = self
      // TODO: Fix the height and centering of the searchBar, it's very tricky.
      navigationItem.titleView = searchBar
    }
    configButtons()
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    dataSource.reload {
      reloadData()
      noMapsContainer.isHidden = !dataSource.isEmpty || Storage.shared().downloadInProgress()
    }
    Storage.shared().add(self)
  }

  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    Storage.shared().remove(self)
  }

  fileprivate func showChildren(_ nodeAttrs: MapNodeAttributes) {
    let vc = storyboard!.instantiateViewController(ofType: DownloadMapsViewController.self)
    vc.mode = dataSource.isSearching ? .available : mode
    vc.dataSource = dataSource.dataSourceFor(nodeAttrs.countryId)
    navigationController?.pushViewController(vc, animated: true)
  }

  fileprivate func showActions(_ nodeAttrs: MapNodeAttributes, in cell: UITableViewCell) {
    let menuTitle = nodeAttrs.nodeName
    let multiparent = nodeAttrs.parentInfo.count > 1
    let message = dataSource.isRoot || multiparent ? nil : nodeAttrs.parentInfo.first?.countryName
    let actionSheet = UIAlertController(title: menuTitle, message: message, preferredStyle: .actionSheet)
    actionSheet.popoverPresentationController?.sourceView = cell
    actionSheet.popoverPresentationController?.sourceRect = cell.bounds

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
          Storage.shared().showNode(nodeAttrs.countryId)
          self.navigationController?.popToRootViewController(animated: true)
        })
      case .download:
        let prefix = nodeAttrs.totalMwmCount == 1 ? L("downloader_download_map") : L("downloader_download_all_button")
        action = UIAlertAction(title: "\(prefix) (\(formattedSize(nodeAttrs.totalSize)))",
                               style: .default,
                               handler: { _ in
                                Storage.shared().downloadNode(nodeAttrs.countryId)
        })
      case .update:
        let size = formattedSize(nodeAttrs.totalUpdateSizeBytes)
        let title = "\(L("downloader_status_outdated")) \(size)"
        action = UIAlertAction(title: title, style: .default, handler: { _ in
          Storage.shared().updateNode(nodeAttrs.countryId)
        })
      case .cancelDownload:
        action = UIAlertAction(title: L("cancel_download"), style: .destructive, handler: { _ in
          Storage.shared().cancelDownloadNode(nodeAttrs.countryId)
        })
      case .retryDownload:
        action = UIAlertAction(title: L("downloader_retry"), style: .destructive, handler: { _ in
          Storage.shared().retryDownloadNode(nodeAttrs.countryId)
        })
      case .delete:
        action = UIAlertAction(title: L("downloader_delete_map"), style: .destructive, handler: { _ in
          Storage.shared().deleteNode(nodeAttrs.countryId)
        })
      }
      actionSheet.addAction(action)
    }
  }

  fileprivate func reloadData() {
    tableView.reloadData()
    configButtons()
  }

  fileprivate func configButtons() {
    downloadAllView.state = .none
    downloadAllView.isSizeHidden = false
    let parentAttributes = dataSource.parentAttributes()
    let error = parentAttributes.nodeStatus == .error || parentAttributes.nodeStatus == .undefined
    let downloading = parentAttributes.nodeStatus == .downloading || parentAttributes.nodeStatus == .inQueue || parentAttributes.nodeStatus == .applying
    switch mode {
    case .available:
      if dataSource.isRoot {
        break
      }
      if error {
        downloadAllView.state = .error
      } else if downloading {
        downloadAllView.state = .dowloading
      } else if parentAttributes.downloadedMwmCount < parentAttributes.totalMwmCount {
        downloadAllView.state = .ready
        downloadAllView.style = .download
        downloadAllView.downloadSize = parentAttributes.totalSize - parentAttributes.downloadedSize
      }
    case .downloaded:
      let isUpdate = parentAttributes.totalUpdateSizeBytes > 0
      let size = isUpdate ? parentAttributes.totalUpdateSizeBytes : parentAttributes.downloadingSize
      if error {
        downloadAllView.state = dataSource.isRoot ? .none : .error
        downloadAllView.downloadSize = parentAttributes.downloadingSize
      } else if downloading && dataSource is DownloadedMapsDataSource {
        downloadAllView.state = .dowloading
        if dataSource.isRoot {
          downloadAllView.style = .download
          downloadAllView.isSizeHidden = true
        }
      } else if isUpdate {
        downloadAllView.state = .ready
        downloadAllView.style = .update
        downloadAllView.downloadSize = size
      }
    @unknown default:
      fatalError()
    }
  }

  @objc func onAddMaps() {
    let vc = storyboard!.instantiateViewController(ofType: DownloadMapsViewController.self)
    if !dataSource.isRoot {
      vc.dataSource = AvailableMapsDataSource(dataSource.getParentCountryId())
    }
    vc.mode = .available
    navigationController?.pushViewController(vc, animated: true)
  }
}

// MARK: - UITableViewDataSource

extension DownloadMapsViewController: UITableViewDataSource {
  func numberOfSections(in tableView: UITableView) -> Int {
    dataSource.numberOfSections() + (hasAddMapSection ? 1 : 0)
  }

  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    if hasAddMapSection && section == dataSource.numberOfSections() {
      return 1
    }
    return dataSource.numberOfItems(in: section)
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    if hasAddMapSection && indexPath.section == dataSource.numberOfSections() {
      let cellType = MWMMapDownloaderButtonTableViewCell.self
      let buttonCell = tableView.dequeueReusableCell(cell: cellType, indexPath: indexPath)
      return buttonCell
    }

    let nodeAttrs = dataSource.item(at: indexPath)
    let cell: MWMMapDownloaderTableViewCell
    if nodeAttrs.hasChildren {
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
    if indexPath.section == dataSource.numberOfSections() {
      return false
    }
    let nodeAttrs = dataSource.item(at: indexPath)
    switch nodeAttrs.nodeStatus {
    case .onDisk, .onDiskOutOfDate, .partly:
      return true
    default:
      return false
    }
  }

  func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
    if editingStyle == .delete {
      let nodeAttrs = dataSource.item(at: indexPath)
      Storage.shared().deleteNode(nodeAttrs.countryId)
    }
  }
}

// MARK: - UITableViewDelegate

extension DownloadMapsViewController: UITableViewDelegate {
  func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    let headerView = MWMMapDownloaderCellHeader()
    if section != dataSource.numberOfSections() {
      headerView.text = dataSource.title(for: section)
    }
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
    if indexPath.section == dataSource.numberOfSections() {
      onAddMaps()
      return
    }
    let nodeAttrs = dataSource.item(at: indexPath)
    if nodeAttrs.hasChildren {
      showChildren(dataSource.item(at: indexPath))
      return
    }
    showActions(nodeAttrs, in: tableView.cellForRow(at: indexPath)!)
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
    switch nodeAttrs.nodeStatus {
    case .undefined, .error:
      Storage.shared().retryDownloadNode(nodeAttrs.countryId)
    case .downloading, .applying, .inQueue:
      Storage.shared().cancelDownloadNode(nodeAttrs.countryId)
    case .onDiskOutOfDate:
      Storage.shared().updateNode(nodeAttrs.countryId)
    case .onDisk:
      // do nothing
      break
    case .notDownloaded, .partly:
      if nodeAttrs.hasChildren {
        showChildren(nodeAttrs)
      } else {
        Storage.shared().downloadNode(nodeAttrs.countryId)
      }
    @unknown default:
      fatalError()
    }
  }

  func mapDownloaderCellDidLongPress(_ cell: MWMMapDownloaderTableViewCell) {
    guard let indexPath = tableView.indexPath(for: cell) else { return }
    let nodeAttrs = dataSource.item(at: indexPath)
    showActions(nodeAttrs, in: cell)
  }
}

// MARK: - StorageObserver

extension DownloadMapsViewController: StorageObserver {
  func processCountryEvent(_ countryId: String) {
    if skipCountryEvent && countryId == dataSource.getParentCountryId() {
      return
    }
    dataSource.reload {
      reloadData()
      noMapsContainer.isHidden = !dataSource.isEmpty || Storage.shared().downloadInProgress()
    }
    if countryId == dataSource.getParentCountryId() {
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

    if countryId == dataSource.getParentCountryId() {
      downloadAllView.downloadProgress = CGFloat(downloadedBytes) / CGFloat(totalBytes)
      downloadAllView.downloadSize = totalBytes
    } else if dataSource.isRoot && dataSource is DownloadedMapsDataSource {
      downloadAllView.state = .dowloading
      downloadAllView.isSizeHidden = true
    }
  }
}

// MARK: - UISearchBarDelegate

extension DownloadMapsViewController: UISearchBarDelegate {
  func searchBarCancelButtonClicked(_ searchBar: UISearchBar) {
    searchBar.text = nil
    searchBar.resignFirstResponder()
    dataSource.cancelSearch()
    reloadData()
    noSerchResultViewController.view.isHidden = true
  }

  func searchBar(_ searchBar: UISearchBar, textDidChange searchText: String) {
    let locale = searchBar.textInputMode?.primaryLanguage
    dataSource.search(searchText, locale: locale ?? "") { [weak self] finished in
      guard let self = self else { return }
      self.reloadData()
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

// MARK: - DownloadAllViewDelegate

extension DownloadMapsViewController: DownloadAllViewDelegate {
  func onStateChanged(state: DownloadAllView.State) {
    if state == .none {
      downloadAllViewContainer.isHidden = true
      tableView.contentInset = UIEdgeInsets.zero
    } else {
      downloadAllViewContainer.isHidden = false
      tableView.contentInset = UIEdgeInsets(top: 0, left: 0, bottom: allMapsViewBottomOffsetConstant, right: 0)
    }
  }

  func onDownloadButtonPressed() {
    skipCountryEvent = true
    let id = dataSource.getParentCountryId()
    if mode == .downloaded {
      Storage.shared().updateNode(id)
    } else {
      Storage.shared().downloadNode(id)
    }
    skipCountryEvent = false
    processCountryEvent(id)
  }

  func onRetryButtonPressed() {
    skipCountryEvent = true
    let id = dataSource.getParentCountryId()
    Storage.shared().retryDownloadNode(id)
    skipCountryEvent = false
    processCountryEvent(id)
  }

  func onCancelButtonPressed() {
    skipCountryEvent = true
    let id = dataSource.getParentCountryId()
    Storage.shared().cancelDownloadNode(id)
    skipCountryEvent = false
    processCountryEvent(id)
    reloadData()
  }
}
