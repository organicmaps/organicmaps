@objc
enum SearchOnMapState: Int {
  case searching
  case hidden
  case closed
}

@objcMembers
final class SearchOnMapManager: NSObject {
  private let navigationController: UINavigationController
  private weak var interactor: SearchOnMapInteractor?

  weak var viewController: UIViewController?
  var isSearching: Bool { viewController != nil }

  init(navigationController: UINavigationController = MapViewController.shared()!.navigationController!) {
    self.navigationController = navigationController
  }

  // MARK: - Public
  func setState(_ state: SearchOnMapState) {
    switch state {
    case .searching:
      openSearch()
    case .hidden:
      hideSearch()
    case .closed:
      closeSearch()
    }
  }

  func setRoutingTooltip(_ tooltip: MWMSearchManagerRoutingTooltipSearch) {
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

  // MARK: - Private
  private func openSearch() {
    if viewController != nil {
      interactor?.handle(.openSearch)
      return
    }
    FrameworkHelper.deactivateMapSelection()
    let viewController = SearchOnMapBuilder.build()
    self.viewController = viewController
    self.interactor = viewController.interactor
    navigationController.present(viewController, animated: true)
  }

  private func closeSearch() {
    interactor?.handle(.closeSearch)
  }

  private func hideSearch() {
    interactor?.handle(.hideSearch)
  }
}

private struct SearchOnMapBuilder {
  static func build() -> SearchOnMapViewController {
    let transitioningManager = SearchOnMapModalTransitionManager()
    let presenter = SearchOnMapPresenter(transitionManager: transitioningManager)
    let interactor = SearchOnMapInteractor(presenter: presenter)
    let viewController = SearchOnMapViewController(interactor: interactor)
    presenter.view = viewController
    viewController.modalPresentationStyle = .custom
    viewController.transitioningDelegate = transitioningManager
    return viewController
  }
}
