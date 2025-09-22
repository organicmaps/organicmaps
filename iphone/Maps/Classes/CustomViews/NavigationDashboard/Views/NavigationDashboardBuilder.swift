@objcMembers
final class NavigationDashboardBuilder: NSObject {
  static func build(delegate: MWMRoutePreviewDelegate & RouteNavigationControlsDelegate) -> NavigationDashboardViewController {
    let viewController = NavigationDashboardViewController()
    let presenter = NavigationDashboard.Presenter(view: viewController)
    let interactor = NavigationDashboard.Interactor(presenter: presenter)
    interactor.delegate = delegate
    viewController.interactor = interactor
    return viewController
  }
}
