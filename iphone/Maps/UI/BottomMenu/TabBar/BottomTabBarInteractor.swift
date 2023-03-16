protocol BottomTabBarInteractorProtocol: AnyObject {
  func openSearch()
  func openPoint2Point()
  func openHelp()
  func openBookmarks()
  func openMenu()
}

class BottomTabBarInteractor {
  weak var presenter: BottomTabBarPresenterProtocol?
  private weak var viewController: UIViewController?
  private weak var mapViewController: MapViewController?
  private weak var controlsManager: MWMMapViewControlsManager?
  private weak var searchManager = MWMSearchManager.manager()
  
  init(viewController: UIViewController, mapViewController: MapViewController, controlsManager: MWMMapViewControlsManager) {
    self.viewController = viewController
    self.mapViewController = mapViewController
    self.controlsManager = controlsManager
  }
}

extension BottomTabBarInteractor: BottomTabBarInteractorProtocol {
  func openSearch() {
    if searchManager?.state == .hidden {
      searchManager?.state = .default
    } else {
      searchManager?.state = .hidden
    }
  }

  func openPoint2Point() {
    MWMRouter.enableAutoAddLastLocation(false)
    // Is stopRouting really needed here?
    MWMRouter.stopRouting()
    controlsManager?.onRoutePrepare()
  }
  
  func openHelp() {
    MapViewController.shared()?.navigationController?.pushViewController(AboutController(), animated: true)
  }
  
  func openBookmarks() {
    mapViewController?.bookmarksCoordinator.open()
  }
  
  func openMenu() {
    guard let state = controlsManager?.menuState else {
      fatalError()
    }
    switch state {
    case .inactive: controlsManager?.menuState = .active
    case .active: controlsManager?.menuState = .inactive
    case .hidden: fallthrough
    case .layers: fallthrough
    @unknown default: fatalError()
    }
  }
}
