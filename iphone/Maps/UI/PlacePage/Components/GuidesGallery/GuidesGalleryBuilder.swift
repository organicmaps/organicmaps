@objc(MWMGuidesGalleryBuilder)
final class GuidesGalleryBuilder: NSObject {
  @objc static func build() -> GuidesGalleryViewController {
    let placePageData = PlacePageData(localizationProvider: OpeinigHoursLocalization())
    guard let guidesGalleryData = placePageData.guidesGalleryData else {
      fatalError()
    }

    let storyboard = UIStoryboard.instance(.placePage)
    let viewController = storyboard.instantiateViewController(ofType: GuidesGalleryViewController.self);
    let presenter = GuidesGalleryPresenter(view: viewController, guidesGallery: guidesGalleryData)
    viewController.presenter = presenter
    return viewController
  }
}
