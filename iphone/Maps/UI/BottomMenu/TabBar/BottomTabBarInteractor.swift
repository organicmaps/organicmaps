protocol BottomTabBarInteractorProtocol: AnyObject {
  func openSearch()
  func openHelp()
  func openFaq()
  func openBookmarks()
  func openTrackRecorder()
  func openMenu()
  func startObservingTrackRecordingState()
}

class BottomTabBarInteractor {
  weak var presenter: BottomTabBarPresenterProtocol?
  private weak var viewController: BottomTabBarViewController?
  private weak var mapViewController: MapViewController?
  private weak var controlsManager: MWMMapViewControlsManager?
  private let searchManager: SearchOnMapManager
  private let trackRecordingManager: TrackRecordingManager

  init(viewController: BottomTabBarViewController, mapViewController: MapViewController, controlsManager: MWMMapViewControlsManager) {
    self.viewController = viewController
    self.mapViewController = mapViewController
    self.controlsManager = controlsManager
    self.searchManager = mapViewController.searchManager
    self.trackRecordingManager = mapViewController.trackRecordingManager
  }
}

extension BottomTabBarInteractor: BottomTabBarInteractorProtocol {
  func openSearch() {
    searchManager.isSearching ? searchManager.close() : searchManager.startSearching(isRouting: false)
  }
  
  func openHelp() {
    MapViewController.shared()?.navigationController?.pushViewController(AboutController(), animated: true)
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

  func openTrackRecorder() {
    switch trackRecordingManager.recordingState {
    case .inactive:
      trackRecordingManager.start { [weak self] result in
        guard let self else { return }
        switch result {
        case .failure:
          break
        case .success:
          self.mapViewController?.showTrackRecordingPlacePage()
        }
      }
    case .active:
      mapViewController?.showTrackRecordingPlacePage()
    }
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

  func startObservingTrackRecordingState() {
    self.trackRecordingManager.addObserver(self) { [weak self] state, _, _ in
      self?.viewController?.trackRecordingState = state
    }
  }
}
