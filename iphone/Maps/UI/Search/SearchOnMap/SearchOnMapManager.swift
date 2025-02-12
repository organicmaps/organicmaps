@objc
enum SearchOnMapState: Int {
  case searching
  case hidden
  case closed
}

@objc
enum SearchOnMapRoutingTooltipSearch: Int {
  case none
  case start
  case finish
}

@objc
protocol SearchOnMapManagerObserver: AnyObject {
  func searchManager(didChangeState state: SearchOnMapState)
}

@objcMembers
final class SearchOnMapManager: NSObject {
  private let navigationController: UINavigationController
  private weak var interactor: SearchOnMapInteractor?
  private let observers = ListenerContainer<SearchOnMapManagerObserver>()

  // MARK: - Public properties
  weak var viewController: UIViewController?
  var isSearching: Bool { viewController != nil }

  init(navigationController: UINavigationController = MapViewController.shared()!.navigationController!) {
    self.navigationController = navigationController
  }

  // MARK: - Public methods
  func startSearching(isRouting: Bool) {
    if viewController != nil {
      interactor?.handle(.openSearch)
      return
    }
    FrameworkHelper.deactivateMapSelection()
    let viewController = buildViewController(isRouting: isRouting)
    self.viewController = viewController
    self.interactor = viewController.interactor
    navigationController.present(viewController, animated: true)
  }

  func hide() {
    interactor?.handle(.hideSearch)
  }

  func close() {
    interactor?.handle(.closeSearch)
  }

  func setRoutingTooltip(_ tooltip: SearchOnMapRoutingTooltipSearch) {
    interactor?.routingTooltipSearch = tooltip
  }

  func setPlaceOnMapSelected(_ isSelected: Bool) {
    interactor?.handle(isSelected ? .didSelectPlaceOnMap : .didDeselectPlaceOnMap)
  }

  func setMapIsDragging() {
    interactor?.handle(.didStartDraggingMap)
  }

  func searchText(_ text: String, locale: String, isCategory: Bool) {
    let searchText = SearchOnMap.SearchText(text, locale: locale)
    interactor?.handle(.didSelectText(searchText, isCategory: isCategory))
  }

  func addObserver(_ observer: SearchOnMapManagerObserver) {
    observers.addListener(observer)
  }

  func removeObserver(_ observer: SearchOnMapManagerObserver) {
    observers.removeListener(observer)
  }

  // MARK: - Private methods
  private func buildViewController(isRouting: Bool) -> SearchOnMapViewController {
    let transitioningManager = SearchOnMapModalTransitionManager()
    let presenter = SearchOnMapPresenter(transitionManager: transitioningManager,
                                         isRouting: isRouting,
                                         didChangeState: { [weak self] state in
      guard let self else { return }
      self.observers.forEach { observer in observer.searchManager(didChangeState: state) }
    })
    let interactor = SearchOnMapInteractor(presenter: presenter)
    let viewController = SearchOnMapViewController(interactor: interactor)
    presenter.view = viewController
    viewController.modalPresentationStyle = .custom
    viewController.transitioningDelegate = transitioningManager
    return viewController
  }
}
