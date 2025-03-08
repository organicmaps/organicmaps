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
  private var interactor: SearchOnMapInteractor? { viewController?.interactor }
  private let observers = ListenerContainer<SearchOnMapManagerObserver>()

  weak var viewController: SearchOnMapViewController?
  var isSearching: Bool { viewController != nil }

  override init() {
    super.init()
  }

  // MARK: - Public methods
  func startSearching(isRouting: Bool) {
    if viewController != nil {
      interactor?.handle(.openSearch)
      return
    }
    FrameworkHelper.deactivateMapSelection()
    let viewController = SearchOnMapViewControllerBuilder.build(isRouting: isRouting,
                                                                didChangeState: notifyObservers)
    self.viewController = viewController
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

  private func notifyObservers(_ state: SearchOnMapState) {
    observers.forEach { observer in observer.searchManager(didChangeState: state) }
  }
}

private struct SearchOnMapViewControllerBuilder {
  static func build(isRouting: Bool, didChangeState: @escaping ((SearchOnMapState) -> Void)) -> SearchOnMapViewController {
    let mapViewController = MapViewController.shared()!
    let presentationController = SearchOnMapPresentationController(parentViewController: mapViewController,
                                                            containerView: mapViewController.searchContainer)
    let viewController = SearchOnMapViewController(presentationController: presentationController)
    let presenter = SearchOnMapPresenter(isRouting: isRouting,
                                         didChangeState: didChangeState)
    let interactor = SearchOnMapInteractor(presenter: presenter)
    presenter.view = viewController
    viewController.interactor = interactor
    presentationController.show()
    return viewController
  }
}
