import CoreApi

class ElevationProfileBuilder {
  static func build(trackData: PlacePageTrackData,
                    delegate: ElevationProfileViewControllerDelegate?) -> ElevationProfileViewController {
    let viewController = ElevationProfileViewController();
    let presenter = ElevationProfilePresenter(view: viewController,
                                              trackData: trackData,
                                              delegate: delegate)
    viewController.presenter = presenter
    return viewController
  }
}
