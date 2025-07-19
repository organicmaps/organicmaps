protocol BottomMenuPresenterProtocol: UITableViewDelegate, UITableViewDataSource {
  func onClosePressed()
}

class BottomMenuPresenter: NSObject {
  enum CellType: Int, CaseIterable {
    case addPlace
    case downloadMaps
    case donate
    case settings
    case share
  }

  enum Sections: Int {
    case layers
    case items
  }

  private weak var view: BottomMenuViewProtocol?
  private let interactor: BottomMenuInteractorProtocol
  private let sections: [Sections]
  private let menuCells: [CellType]
  private let trackRecorder = TrackRecordingManager.shared

  init(view: BottomMenuViewProtocol,
       interactor: BottomMenuInteractorProtocol,
       sections: [Sections]) {
    self.view = view
    self.interactor = interactor
    self.sections = sections
    let disableDonate = Settings.donateUrl() == nil
    self.menuCells = CellType.allCases.filter { disableDonate ? $0 != .donate : true }
    super.init()
  }
}

extension BottomMenuPresenter: BottomMenuPresenterProtocol {
  func onClosePressed() {
    interactor.close()
  }
}

//MARK: -- UITableViewDataSource

extension BottomMenuPresenter {
  func numberOfSections(in tableView: UITableView) -> Int {
    sections.count
  }

  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    switch sections[section] {
    case .layers:
      return 1
    case .items:
      return menuCells.count
    }
  }
  
  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    switch sections[indexPath.section] {
    case .layers:
      let cell = tableView.dequeueReusableCell(cell: BottomMenuLayersCell.self)!
      cell.onClose = { [weak self] in self?.onClosePressed() }
      if sections.count > 1 {
        cell.addSeparator(.bottom)
      }
      return cell
    case .items:
      let cell = tableView.dequeueReusableCell(cell: BottomMenuItemCell.self)!
      switch menuCells[indexPath.row] {
      case .addPlace:
        let enabled = MWMNavigationDashboardManager.shared().state == .hidden && FrameworkHelper.canEditMapAtViewportCenter()
        cell.configure(imageName: "ic_add_place",
                       title: L("placepage_add_place_button"),
                       enabled: enabled)
        return cell
      case .downloadMaps:
        cell.configure(imageName: "ic_menu_download",
                       title: L("download_maps"),
                       badgeCount: MapsAppDelegate.theApp().badgeNumber())
      case .donate:
        cell.configure(imageName: Settings.isNY() ? "ic_christmas_tree" : "ic_menu_donate",
                       title: L("donate"))
      case .settings:
        cell.configure(imageName: "ic_menu_settings",
                       title: L("settings"))
      case .share:
        cell.configure(imageName: "ic_menu_share",
                       title: L("share_my_location"))
      }
      return cell
    }
  }
}

//MARK: -- UITableViewDelegate

extension BottomMenuPresenter {
  func tableView(_ tableView: UITableView, willSelectRowAt indexPath: IndexPath) -> IndexPath? {
    if let cell = tableView.cellForRow(at: indexPath) as? BottomMenuItemCell {
      return cell.isEnabled ? indexPath : nil
    }
    return indexPath
  }

  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    switch sections[indexPath.section] {
    case .layers:
      return;
    case .items:
      switch menuCells[indexPath.row] {
      case .addPlace:
        interactor.addPlace()
      case .downloadMaps:
        interactor.downloadMaps()
      case .donate:
        interactor.donate()
      case .settings:
        interactor.openSettings()
      case .share:
        if let cell = tableView.cellForRow(at: indexPath) as? BottomMenuItemCell {
          interactor.shareLocation(cell: cell)
        }
      }
    }
  }
}
