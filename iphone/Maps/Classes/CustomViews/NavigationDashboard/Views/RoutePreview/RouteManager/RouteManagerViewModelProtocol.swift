@objc(MWMRouteManagerViewModelProtocol)
protocol RouteManagerViewModelProtocol: class {
  var routePoints: [MWMRoutePoint] { get }

  var refreshControlsCallback: (() -> Void)! { get set }
  var reloadCallback: (() -> Void)! { get set }

  func startTransaction()
  func finishTransaction()
  func cancelTransaction()

  func addLocationPoint()
  func movePoint(at index: Int, to newIndex: Int)
  func deletePoint(at index: Int)
}
