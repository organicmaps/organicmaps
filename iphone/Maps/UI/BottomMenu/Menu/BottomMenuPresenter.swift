protocol BottomMenuPresenterProtocol: UITableViewDelegate, UITableViewDataSource {
  func onClosePressed()
}

class BottomMenuPresenter: NSObject {
  enum CellType: Int, CaseIterable {
    case addPlace
    case downloadRoutes
    case downloadMaps
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
  private let source: String

  init(view: BottomMenuViewProtocol, 
       interactor: BottomMenuInteractorProtocol,
       sections: [Sections],
       source: String) {
    self.view = view
    self.interactor = interactor
    self.sections = sections
    self.source = source
  }
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
    section == Sections.layers.rawValue ? 1 : CellType.allCases.count
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    if indexPath.section == Sections.layers.rawValue {
      let cell = tableView.dequeueReusableCell(cell: BottomMenuLayersCell.self)!
      cell.onClose = { [weak self] in self?.onClosePressed() }
      cell.source = source
      return cell
    }
    if indexPath.section == Sections.items.rawValue {
      let cell = tableView.dequeueReusableCell(cell: BottomMenuItemCell.self)!
      switch CellType(rawValue: indexPath.row)! {
      case .addPlace:
        let enabled = MWMNavigationDashboardManager.shared().state == .hidden && FrameworkHelper.canEditMap()
        cell.configure(imageName: "ic_add_place",
                       title: L("placepage_add_place_button"),
                       badgeCount: 0,
                       enabled: enabled)
      case .downloadRoutes:
        cell.configure(imageName: "ic_menu_routes", title: L("download_guides"))
      case .downloadMaps:
        cell.configure(imageName: "ic_menu_download",
                       title: L("download_maps"),
                       badgeCount: MapsAppDelegate.theApp().badgeNumber(),
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
    fatalError()
  }

  func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    return section > 0 ? 12 : 0
  }

  func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    if section > 0 {
      let view = UIView()
      view.styleName = "BlackOpaqueBackground";
      return view;
    }
    return nil
  }
}


//MARK: -- UITableDelegate

extension BottomMenuPresenter {
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    guard indexPath.section == Sections.items.rawValue else {
      return
    }
    tableView.deselectRow(at: indexPath, animated: true)
    switch CellType(rawValue: indexPath.row)! {
    case .addPlace:
      interactor.addPlace()
    case .downloadRoutes:
      interactor.downloadRoutes()
    case .downloadMaps:
      interactor.downloadMaps()
    case .settings:
      interactor.openSettings()
    case .share:
      if let cell = tableView.cellForRow(at: indexPath) as? BottomMenuItemCell {
        interactor.shareLocation(cell: cell)
      }
    }
  }
}
