@objcMembers
final class RoutePreviewBuilder: NSObject {
  static func build(delegate: MWMRoutePreviewDelegate & RouteNavigationControlsDelegate) -> RoutePreviewViewController {
    let viewController = RoutePreviewViewController()
    let presenter = RoutePreview.Presenter(view: viewController)
    let interactor = RoutePreview.Interactor(presenter: presenter)
    interactor.delegate = delegate
    viewController.interactor = interactor
    return viewController
  }
}
