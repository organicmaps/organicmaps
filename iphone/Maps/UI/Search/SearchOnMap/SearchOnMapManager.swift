@objc
enum SearchOnMapState: Int {
  case searching
  case hidden
  case closed
}

@objcMembers
final class SearchOnMapManager: NSObject {
  static let shared = SearchOnMapManager()

  private let navigationController: UINavigationController
  private weak var viewController: SearchOnMapViewController?
  private weak var interactor: SearchOnMapInteractor?

  init(navigationController: UINavigationController = MapViewController.shared()!.navigationController!) {
    self.navigationController = navigationController
  }

  var isSearching: Bool { viewController != nil }

  @objc
  func setState(_ state: SearchOnMapState) {
    switch state {
    case .searching:
      beginSearch()
    case .hidden:
      hide()
    case .closed:
      close()
    }
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

  private func beginSearch() {
    if viewController != nil {
      interactor?.handle(.didStartTyping)
      return
    }
    FrameworkHelper.deactivateMapSelection()

    let transitioningManager = SearchOnMapModalTransitionManager()
    let presenter = SearchOnMapPresenter(transitionManager: transitioningManager)
    let interactor = SearchOnMapInteractor(presenter: presenter)
    let viewController = SearchOnMapViewController(interactor: interactor)
    self.interactor = interactor
    self.viewController = viewController
    presenter.view = viewController
    viewController.modalPresentationStyle = .custom
    viewController.transitioningDelegate = transitioningManager
    navigationController.present(viewController, animated: true)
  }

  private func close() {
    interactor?.handle(.closeSearch)
  }

  private func hide() {
    interactor?.handle(.didSelectPlaceOnMap)
  }
}
