@objc class PlacePageBuilder: NSObject {    
  @objc static func build(for data: PlacePageData) -> PlacePageViewController {
    let storyboard = UIStoryboard.instance(.placePage)
    guard let viewController = storyboard.instantiateInitialViewController() as? PlacePageViewController else {
      fatalError()
    }
    viewController.isPreviewPlus = data.isPreviewPlus
    let interactor = PlacePageInteractor(data: data)
    let layout: IPlacePageLayout
    switch data.objectType {
    case .POI, .bookmark:
      layout = PlacePageCommonLayout(interactor: interactor, storyboard: storyboard, data: data)
    case .track:
      let trackLayout = PlacePageTrackLayout(interactor: interactor, storyboard: storyboard, data: data)
      interactor.trackActivePointPresenter = trackLayout.elevationMapViewController?.presenter
      layout = trackLayout
    case .trackRecording:
      layout = PlacePageTrackRecordingLayout(interactor: interactor, storyboard: storyboard, data: data)
    @unknown default:
      fatalError()
    }
    let presenter = PlacePagePresenter(view: viewController,
                                       headerView: layout.headerViewController,
                                       mapViewController: MapViewController.shared()!)
    viewController.layout = layout
    viewController.interactor = interactor
    interactor.presenter = presenter
    layout.presenter = presenter
    return viewController
	}

  @objc static func update(_ viewController: PlacePageViewController, with data: PlacePageData) {
    viewController.isPreviewPlus = data.isPreviewPlus
    let interactor = PlacePageInteractor(data: data)
    let layout: IPlacePageLayout
    let storyboard = viewController.storyboard!
    switch data.objectType {
    case .POI, .bookmark:
      layout = PlacePageCommonLayout(interactor: interactor, storyboard: storyboard, data: data)
    case .track:
      let trackLayout = PlacePageTrackLayout(interactor: interactor, storyboard: storyboard, data: data)
      interactor.trackActivePointPresenter = trackLayout.elevationMapViewController?.presenter
      layout = trackLayout
    case .trackRecording:
      layout = PlacePageTrackRecordingLayout(interactor: interactor, storyboard: storyboard, data: data)
    @unknown default:
      fatalError()
    }
    let presenter = PlacePagePresenter(view: viewController,
                                       headerView: layout.headerViewController,
                                       mapViewController: MapViewController.shared()!)
    viewController.interactor = interactor
    interactor.presenter = presenter
    layout.presenter = presenter
    viewController.updateWithLayout(layout)
  }
}
