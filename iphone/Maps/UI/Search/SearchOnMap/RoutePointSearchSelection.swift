protocol RoutePointSelecting: AnyObject {
  var isActive: Bool { get }
  var title: String { get }
  var canSelectCurrentLocation: Bool { get }

  func selectCurrentLocation() -> Bool
  func select(searchResult: SearchResult) -> Bool
  func select(mapPoint: CGPoint) -> Bool
  func cancel()
}

final class RoutePointSearchSelection: RoutePointSelecting {
  private let navigationManager: MWMNavigationDashboardManager

  init(navigationManager: MWMNavigationDashboardManager = .shared()) {
    self.navigationManager = navigationManager
  }

  var isActive: Bool { navigationManager.isRoutePointSelectionActive }
  var title: String { navigationManager.routePointSelectionTitle }
  var canSelectCurrentLocation: Bool { navigationManager.canSelectCurrentLocation }

  func selectCurrentLocation() -> Bool {
    navigationManager.selectCurrentLocationForRoute()
  }

  func select(searchResult: SearchResult) -> Bool {
    select(point: searchResult.point,
           title: searchResult.titleText,
           subtitle: searchResult.addressText)
  }

  func select(mapPoint: CGPoint) -> Bool {
    select(point: mapPoint, title: nil, subtitle: nil)
  }

  func cancel() {
    navigationManager.cancelRoutePointSelection()
  }

  private func select(point: CGPoint, title: String?, subtitle: String?) -> Bool {
    navigationManager.selectRoutePoint(at: point, title: title, subtitle: subtitle)
  }
}
