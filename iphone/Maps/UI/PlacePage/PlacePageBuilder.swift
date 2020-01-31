@objc class PlacePageBuilder: NSObject {    
  @objc static func build(data: PlacePageData) -> UIViewController {
    let storyboard = UIStoryboard.instance(.placePage)
    guard let viewController = storyboard.instantiateInitialViewController() as? PlacePageViewController else {
      fatalError()
    }
    let interactor = PlacePageInteractor(viewController: viewController, data: data)
    let layout:IPlacePageLayout
    if data.elevationProfileData != nil {
      layout = PlacePageElevationLayout(interactor: interactor, storyboard: storyboard, data: data)
    } else {
      layout = PlacePageCommonLayout(interactor: interactor, storyboard: storyboard, data: data)
    }
    let presenter = PlacePagePresenter(view: viewController,
                                       interactor: interactor,
                                       layout: layout,
                                       isPreviewPlus: data.isPreviewPlus)

    interactor.presenter = presenter
    viewController.presenter = presenter
    layout.presenter = presenter
    
    return viewController
	}
}
