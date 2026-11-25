import CoreApi

class ElevationProfileBuilder {
  private enum Constants {
    static let minPointsToDraw = 2
  }

  static func build(state: ElevationProfileState?,
                    delegate: ElevationProfileViewControllerDelegate?,
                    presentationStyle: ElevationProfileViewController.PresentationStyle) -> ElevationProfileViewController {
    let viewController = ElevationProfileViewController(presentationStyle: presentationStyle)
    let presenter = ElevationProfilePresenter(view: viewController,
                                              state: state,
                                              delegate: delegate)
    viewController.presenter = presenter
    return viewController
  }

  static func build(trackData: PlacePageTrackData,
                    delegate: ElevationProfileViewControllerDelegate?,
                    presentationStyle: ElevationProfileViewController.PresentationStyle) -> ElevationProfileViewController? {
    guard let data = makeTrackDisplayData(trackData: trackData) else {
      return nil
    }
    return build(state: .track(data), delegate: delegate, presentationStyle: presentationStyle)
  }

  static func makeRoutePreviewState(routeElevationPreviewData: RouteElevationPreviewData,
                                    activePointDistance: Double = 0) -> ElevationProfileState? {
    guard var chartData = makeChartData(elevationProfileData: routeElevationPreviewData.elevationProfileData) else {
      return nil
    }
    if routeElevationPreviewData.finishPointIndex >= 0 {
      chartData.highlightedSection = (routeElevationPreviewData.entryPointIndex, routeElevationPreviewData.finishPointIndex)
    }
    let data = ElevationProfileDisplayData(trackInfo: routeElevationPreviewData.trackInfo,
                                           chartData: chartData,
                                           activePointDistance: activePointDistance,
                                           myPositionDistance: -1)
    return .routePreview(data)
  }

  static func makeTrackRecordingState(trackData: PlacePageTrackData) -> ElevationProfileState? {
    makeTrackDisplayData(trackData: trackData).map(ElevationProfileState.trackRecording)
  }

  private static func makeTrackDisplayData(trackData: PlacePageTrackData) -> ElevationProfileDisplayData? {
    guard trackData.trackInfo.hasElevationInfo,
          let chartData = makeChartData(elevationProfileData: trackData.elevationProfileData) else {
      return nil
    }
    return ElevationProfileDisplayData(trackInfo: trackData.trackInfo,
                                       chartData: chartData,
                                       activePointDistance: trackData.activePointDistance,
                                       myPositionDistance: trackData.myPositionDistance)
  }

  private static func makeChartData(elevationProfileData: ElevationProfileData?) -> ElevationProfileChartData? {
    guard let elevationProfileData else {
      return nil
    }
    let chartData = ElevationProfileChartData(elevationProfileData)
    guard chartData.pointsCount >= Constants.minPointsToDraw else {
      return nil
    }
    return chartData
  }
}
