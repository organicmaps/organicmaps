protocol BottomTabBarInteractorProtocol: class {
  func openSearch()
  func openPoint2Point()
  func openDiscovery()
  func openBookmarks()
  func openMenu()
}

class BottomTabBarInteractor {
  weak var presenter: BottomTabBarPresenterProtocol?
  private weak var viewController: UIViewController?
  private weak var mapViewController: MapViewController?
  private weak var controlsManager: MWMMapViewControlsManager?
  
  private weak var searchManager = MWMSearchManager.manager()
  
  private var isPoint2PointSelected = false
  
  
  
  init(viewController: UIViewController,
       mapViewController: MapViewController,
       controlsManager: MWMMapViewControlsManager) {
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
    isPoint2PointSelected.toggle()
    MWMRouter.enableAutoAddLastLocation(false)
    if (isPoint2PointSelected) {
      controlsManager?.onRoutePrepare()
    } else {
      MWMRouter.stopRouting()
    }
  }
  
  func openDiscovery() {
    NetworkPolicy.shared().callOnlineApi { (canUseNetwork) in
      let vc = MWMDiscoveryController.instance(withConnection: canUseNetwork)
      MapViewController.shared()?.navigationController?.pushViewController(vc!, animated: true)
    }
  }
  
  func openBookmarks() {
    mapViewController?.bookmarksCoordinator.open()
  }
  
  func openMenu() {
    guard let state = controlsManager?.menuState else {
      fatalError()
    }
    switch state {
    case .hidden: assertionFailure("Incorrect state")
    case .inactive:
      controlsManager?.menuState = .active
    case .active:
      controlsManager?.menuState = .inactive
    @unknown default:
      fatalError()
    }
  }
}
