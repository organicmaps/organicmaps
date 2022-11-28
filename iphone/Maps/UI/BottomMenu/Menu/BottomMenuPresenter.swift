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

  init(view: BottomMenuViewProtocol, 
       interactor: BottomMenuInteractorProtocol,
       sections: [Sections]) {
    self.view = view
    self.interactor = interactor
    self.sections = sections
  }
  
  let disableDonate = Settings.donateUrl() == nil
}

extension BottomMenuPresenter: BottomMenuPresenterProtocol {
  func onClosePressed() {
    interactor.close()
  }
}

//MARK: -- UITableDataSource

extension BottomMenuPresenter {
  func numberOfSections(in tableView: UITableView) -> Int {
    sections.count
  }

  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    section == Sections.layers.rawValue ? 1 : CellType.allCases.count - (disableDonate ? 1 : 0)
  }

  private func correctedRow(_ row: Int) -> Int {
    disableDonate && row >= CellType.donate.rawValue ? row + 1 : row
  }
  
  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    if indexPath.section == Sections.layers.rawValue {
      let cell = tableView.dequeueReusableCell(cell: BottomMenuLayersCell.self)!
      cell.onClose = { [weak self] in self?.onClosePressed() }
      return cell
    }
    let cell = tableView.dequeueReusableCell(cell: BottomMenuItemCell.self)!
    switch CellType(rawValue: correctedRow(indexPath.row))! {
    case .addPlace:
      let enabled = MWMNavigationDashboardManager.shared().state == .hidden && FrameworkHelper.canEditMap()
      cell.configure(imageName: "ic_add_place",
                     title: L("placepage_add_place_button"),
                     badgeCount: 0,
                     enabled: enabled)
    case .downloadMaps:
      cell.configure(imageName: "ic_menu_download",
                     title: L("download_maps"),
                     badgeCount: MapsAppDelegate.theApp().badgeNumber(),
                     enabled: true)
    case .donate:
      cell.configure(imageName: "ic_menu_donate",
                     title: L("donate"),
                     badgeCount: 0,
                     enabled: true)
    case .settings:
      cell.configure(imageName: "ic_menu_settings",
                     title: L("settings"),
                     badgeCount: 0,
                     enabled: true)
    case .share:
      cell.configure(imageName: "ic_menu_share",
                     title: L("share_my_location"),
                     badgeCount: 0,
                     enabled: true)
    }
    return cell
  }
}


//MARK: -- UITableDelegate

extension BottomMenuPresenter {
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    guard indexPath.section == Sections.items.rawValue else {
      return
    }
    tableView.deselectRow(at: indexPath, animated: true)
    switch CellType(rawValue: correctedRow(indexPath.row))! {
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
