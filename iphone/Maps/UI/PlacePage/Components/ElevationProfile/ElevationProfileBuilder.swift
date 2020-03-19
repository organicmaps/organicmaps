class ElevationProfileBuilder {
  static func build(data: PlacePageData, delegate: ElevationProfileViewControllerDelegate?) -> ElevationProfileViewController {
    guard let elevationProfileData = data.elevationProfileData else {
      fatalError()
    }
    let storyboard = UIStoryboard.instance(.placePage)
    let viewController = storyboard.instantiateViewController(ofType: ElevationProfileViewController.self);
    let presenter = ElevationProfilePresenter(view: viewController,
                                              data: elevationProfileData,
                                              imperialUnits: Settings.measurementUnits() == .imperial,
                                              delegate: delegate)
    
    viewController.presenter = presenter

    return viewController
  }
}
