protocol BottomMenuInteractorProtocol: AnyObject {
  func close()
  func addPlace()
  func downloadMaps()
  func donate()
  func openSettings()
  func shareLocation(cell: BottomMenuItemCell)
}

@objc protocol BottomMenuDelegate {
  func actionDownloadMaps(_ mode: MWMMapDownloaderMode)
  func addPlace()
  func didFinishAddingPlace()
}

class BottomMenuInteractor {
  weak var presenter: BottomMenuPresenterProtocol?
  private weak var viewController: UIViewController?
  private weak var mapViewController: MapViewController?
  private weak var delegate: BottomMenuDelegate?
  private weak var controlsManager: MWMMapViewControlsManager?

  init(viewController: UIViewController,
       mapViewController: MapViewController,
       controlsManager: MWMMapViewControlsManager,
       delegate: BottomMenuDelegate) {
    self.viewController = viewController
    self.mapViewController = mapViewController
    self.delegate = delegate
    self.controlsManager = controlsManager
  }
}

extension BottomMenuInteractor: BottomMenuInteractorProtocol {
  func close() {
    guard let controlsManager = controlsManager else {
      fatalError()
    }
    controlsManager.menuState = controlsManager.menuRestoreState
  }

  func addPlace() {
    delegate?.addPlace()
  }

  func donate() {
    close()
    guard var url = Settings.donateUrl() else { return }
    if url == "https://organicmaps.app/donate/" {
      url = L("translated_om_site_url") + "donate/"
    }
    viewController?.openUrl(url, inSafari: true)
  }

  func downloadMaps() {
    close()
    self.delegate?.actionDownloadMaps(.downloaded)
  }

  func openSettings() {
    close()
    mapViewController?.performSegue(withIdentifier: "Map2Settings", sender: nil)
  }

  func shareLocation(cell: BottomMenuItemCell) {
    let lastLocation = LocationManager.lastLocation()
    guard let coordinates = lastLocation?.coordinate else {
      let alert = UIAlertController(title: L("unknown_current_position"), message: nil, preferredStyle: .alert)
      alert.addAction(UIAlertAction(title: L("ok"), style: .default, handler: nil))
      viewController?.present(alert, animated: true, completion: nil)
      return;
    }
    guard let viewController = viewController else { return }
    let vc = ActivityViewController.share(forMyPosition: coordinates)
    vc?.present(inParentViewController: viewController, anchorView: cell.anchorView)
  }
}
