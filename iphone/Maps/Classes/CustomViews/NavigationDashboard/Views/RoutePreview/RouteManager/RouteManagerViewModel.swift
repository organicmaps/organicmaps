@objc(MWMRouteManagerViewModel)
final class RouteManagerViewModel: NSObject, RouteManagerViewModelProtocol {
  var routePoints: [MWMRoutePoint] { return MWMRouter.points() }
  var refreshControlsCallback: (() -> Void)!

  func startTransaction() { MWMRouter.openRouteManagerTransaction() }

  func finishTransaction() {
    MWMRouter.applyRouteManagerTransaction()
    MWMRouter.rebuild(withBestRouter: false)
  }

  func cancelTransaction() { MWMRouter.cancelRouteManagerTransaction() }

  func addLocationPoint() {
    MWMRouter.addPoint(MWMRoutePoint(lastLocationAndType: .start, intermediateIndex: 0))
    refreshControlsCallback()
  }
  func movePoint(at index: Int, to newIndex: Int) {
    MWMRouter.movePoint(at: index, to: newIndex)
    refreshControlsCallback()
  }
  func deletePoint(at index: Int) {
    MWMRouter.removePoint(routePoints[index])
    refreshControlsCallback()
  }
}
