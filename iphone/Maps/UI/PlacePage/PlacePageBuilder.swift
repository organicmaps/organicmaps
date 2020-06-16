@objc class PlacePageBuilder: NSObject {    
  @objc static func build() -> UIViewController {
    let storyboard = UIStoryboard.instance(.placePage)
    guard let viewController = storyboard.instantiateInitialViewController() as? PlacePageViewController else {
      fatalError()
    }
    let data = PlacePageData(localizationProvider: OpeinigHoursLocalization())
    viewController.isPreviewPlus = data.isPreviewPlus
    let interactor = PlacePageInteractor(viewController: viewController,
                                         data: data,
                                         mapViewController: MapViewController.shared())
    let layout:IPlacePageLayout
    if data.elevationProfileData != nil {
      layout = PlacePageElevationLayout(interactor: interactor, storyboard: storyboard, data: data)
    } else {
      layout = PlacePageCommonLayout(interactor: interactor, storyboard: storyboard, data: data)
    }
    let presenter = PlacePagePresenter(view: viewController,
                                       interactor: interactor,
                                       layout: layout)

    interactor.presenter = presenter
    viewController.presenter = presenter
    layout.presenter = presenter
    
    return viewController
	}
}
