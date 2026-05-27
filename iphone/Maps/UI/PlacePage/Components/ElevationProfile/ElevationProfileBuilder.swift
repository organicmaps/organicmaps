import CoreApi

class ElevationProfileBuilder {
  static func build(routeElevationPreviewData: RouteElevationPreviewData,
                    delegate: ElevationProfileViewControllerDelegate?,
                    presentationStyle: ElevationProfileViewController.PresentationStyle) -> ElevationProfileViewController {
    let viewController = ElevationProfileViewController(presentationStyle: presentationStyle)
    let presenter = ElevationProfilePresenter(view: viewController,
                                              trackInfo: routeElevationPreviewData.trackInfo,
                                              elevationProfileData: routeElevationPreviewData.elevationProfileData,
                                              delegate: delegate)
    viewController.presenter = presenter
    return viewController
  }

  static func build(trackData: PlacePageTrackData,
                    delegate: ElevationProfileViewControllerDelegate?,
                    presentationStyle: ElevationProfileViewController.PresentationStyle) -> ElevationProfileViewController {
    let viewController = ElevationProfileViewController(presentationStyle: presentationStyle)
    let presenter = ElevationProfilePresenter(view: viewController,
                                              trackInfo: trackData.trackInfo,
                                              elevationProfileData: trackData.elevationProfileData,
                                              activePointDistance: trackData.activePointDistance,
                                              myPositionDistance: trackData.myPositionDistance,
                                              delegate: delegate)
    viewController.presenter = presenter
    return viewController
  }
}
