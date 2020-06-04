@objc(MWMGuidesGalleryBuilder)
final class GuidesGalleryBuilder: NSObject {
  @objc static func build() -> GuidesGalleryViewController {
    let storyboard = UIStoryboard.instance(.placePage)
    let viewController = storyboard.instantiateViewController(ofType: GuidesGalleryViewController.self);
    let router = GuidesGalleryRouter(MapViewController.shared())
    let interactor = GuidesGalleryInteractor()
    let presenter = GuidesGalleryPresenter(view: viewController, router: router, interactor: interactor)
    viewController.presenter = presenter
    return viewController
  }
}
