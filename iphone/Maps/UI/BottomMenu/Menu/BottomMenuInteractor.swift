protocol BottomMenuInteractorProtocol: AnyObject {
  func close()
  func addPlace()
  func downloadMaps()
  func donate()
  func openSettings()
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
    viewController?.openUrl(url, externally: true)
  }

  func downloadMaps() {
    close()
    self.delegate?.actionDownloadMaps(.downloaded)
  }

  func openSettings() {
    close()
    mapViewController?.performSegue(withIdentifier: "Map2Settings", sender: nil)
  }
}
