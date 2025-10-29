protocol BottomTabBarInteractorProtocol: AnyObject {
  func configureTabBar()
  func openSearch()
  func openHelp()
  func openFaq()
  func openBookmarks()
  func openMenu()
}

class BottomTabBarInteractor {
  private weak var viewController: BottomTabBarViewController?
  private weak var mapViewController: MapViewController?
  private weak var controlsManager: MWMMapViewControlsManager?
  private let searchManager: SearchOnMapManager

  init(viewController: BottomTabBarViewController, mapViewController: MapViewController, controlsManager: MWMMapViewControlsManager) {
    self.viewController = viewController
    self.mapViewController = mapViewController
    self.controlsManager = controlsManager
    self.searchManager = mapViewController.searchManager
    self.subscribeOnAppLifecycleNotifications()
  }

  private func subscribeOnAppLifecycleNotifications() {
    NotificationCenter.default.addObserver(self, selector: #selector(configureTabBar), name: UIApplication.willEnterForegroundNotification, object: nil)
  }
}

extension BottomTabBarInteractor: BottomTabBarInteractorProtocol {
  @objc
  func configureTabBar() {
    viewController?.updateAboutButtonIcon(isCrowdfunding: Settings.canShowCrowdfundingPromo())
  }

  func openSearch() {
    searchManager.isSearching ? searchManager.close() : searchManager.startSearching(isRouting: false)
  }
  
  func openHelp() {
    let aboutViewController = AboutController(onDidAppearCompletionHandler: configureTabBar)
    MapViewController.shared()?.navigationController?.pushViewController(aboutViewController, animated: true)
  }
  
  func openFaq() {
    guard let navigationController = MapViewController.shared()?.navigationController else { return }
    let aboutController = AboutController(onDidAppearCompletionHandler: {
      navigationController.pushViewController(FaqController(), animated: true)
    })
    navigationController.pushViewController(aboutController, animated: true)
  }
  
  func openBookmarks() {
    mapViewController?.bookmarksCoordinator.open()
  }
  
  func openMenu() {
    guard let state = controlsManager?.menuState else {
      fatalError("ERROR: Failed to retrieve the current MapViewControlsManager's state.")
    }
    switch state {
    case .inactive: controlsManager?.menuState = .active
    case .active: controlsManager?.menuState = .inactive
    case .hidden:
      // When the current controls manager's state is hidden, accidental taps on the menu button during the hiding animation should be skipped.
      break;
    case .layers: fallthrough
    @unknown default: fatalError("ERROR: Unexpected MapViewControlsManager's state: \(state)")
    }
  }
}
