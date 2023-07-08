@objc class ElevationDetailsBuilder: NSObject {
  @objc static func build(data: PlacePageData) -> UIViewController {
    guard let elevationProfileData = data.elevationProfileData else {
      fatalError()
    }
    let viewController = ElevationDetailsViewController(nibName: toString(ElevationDetailsViewController.self), bundle: nil)
    let router = ElevationDetailsRouter(viewController: viewController)
    let presenter = ElevationDetailsPresenter(view: viewController, router: router, data: elevationProfileData)

    viewController.presenter = presenter

    return viewController
  }
}
