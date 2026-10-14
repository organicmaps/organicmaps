@objc
protocol SearchOnMapManagerObserver: AnyObject {
  func searchManager(didChangeState state: SearchOnMapState)
}

private enum SearchOnMapMode {
  case normal
  case routing
  case routePointPicker
}

@objcMembers
final class SearchOnMapManager: NSObject {
  private var interactor: SearchOnMapInteractor? { viewController?.interactor }
  private let observers = ListenerContainer<SearchOnMapManagerObserver>()

  weak var viewController: SearchOnMapViewController?
  var isSearching: Bool { viewController != nil }
  private var mode: SearchOnMapMode = .normal

  override init() {
    super.init()
  }

  // MARK: - Public methods

  func startSearching(isRouting: Bool) {
    let mode: SearchOnMapMode
    if !isRouting {
      mode = .normal
    } else if MWMNavigationDashboardManager.shared().isRoutePointSelectionActive {
      mode = .routePointPicker
    } else {
      mode = .routing
    }
    if viewController != nil {
      if self.mode == mode {
        interactor?.handle(.openSearch)
        return
      }
      interactor?.closeForReplacement()
      viewController = nil
    }
    self.mode = mode
    FrameworkHelper.deactivateMapSelection()
    let viewController = SearchOnMapViewControllerBuilder.build(mode: mode,
                                                                didChangeState: notifyObservers)
    self.viewController = viewController
  }

  func hide() {
    interactor?.handle(.hideSearch)
  }

  func close() {
    interactor?.handle(.closeSearch)
  }

  func setPlaceOnMapSelected(_ isSelected: Bool) {
    interactor?.handle(isSelected ? .didSelectPlaceOnMap : .didDeselectPlaceOnMap)
  }

  func setMapIsDragging() {
    interactor?.handle(.didStartDraggingMap)
  }

  func searchText(_ searchText: SearchQuery) {
    interactor?.handle(.didSelect(searchText))
  }

  func addObserver(_ observer: SearchOnMapManagerObserver) {
    observers.addListener(observer)
  }

  func removeObserver(_ observer: SearchOnMapManagerObserver) {
    observers.removeListener(observer)
  }

  private func notifyObservers(_ state: SearchOnMapState) {
    if state == .closed {
      viewController = nil
    }
    observers.forEach { observer in observer.searchManager(didChangeState: state) }
  }
}

private enum SearchOnMapViewControllerBuilder {
  static func build(mode: SearchOnMapMode,
                    didChangeState: @escaping ((SearchOnMapState) -> Void)) -> SearchOnMapViewController {
    let routePointSelector: RoutePointSelecting? = mode == .routePointPicker ? RoutePointSearchSelection() : nil
    let routePointActions = routePointSelector.map {
      SearchOnMap.ViewModel.RoutePointActions(title: $0.title,
                                              canSelectCurrentLocation: $0.canSelectCurrentLocation)
    }
    let viewController = SearchOnMapViewController()
    let presenter = SearchOnMapPresenter(shouldHideForRouting: mode == .routing,
                                         routePointActions: routePointActions,
                                         didChangeState: didChangeState)
    let interactor = SearchOnMapInteractor(presenter: presenter,
                                           routePointSelector: routePointSelector,
                                           mapViewController: MapViewController.shared())
    presenter.view = viewController
    viewController.interactor = interactor
    viewController.show()
    return viewController
  }
}
