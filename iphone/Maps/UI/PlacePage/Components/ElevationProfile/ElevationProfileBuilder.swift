import CoreApi

class ElevationProfileBuilder {
  private enum Constants {
    static let minPointsToDraw = 2
  }

  static func build(state: ElevationProfileState,
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
    guard let state = makeTrackState(trackData: trackData) else {
      return nil
    }
    return build(state: state, delegate: delegate, presentationStyle: presentationStyle)
  }

  static func build(trackRecordingData trackData: PlacePageTrackData,
                    delegate: ElevationProfileViewControllerDelegate?,
                    presentationStyle: ElevationProfileViewController.PresentationStyle) -> ElevationProfileViewController {
    build(state: makeTrackRecordingState(trackData: trackData), delegate: delegate, presentationStyle: presentationStyle)
  }

  static func makeRoutePreviewState(routeElevationPreviewData: RouteElevationPreviewData,
                                    activePointDistance: Double = 0) -> ElevationProfileState? {
    guard let chartData = makeChartData(elevationProfileData: routeElevationPreviewData.elevationProfileData) else {
      return nil
    }
    let data = ElevationProfileDisplayData(trackInfo: routeElevationPreviewData.trackInfo,
                                           chartData: chartData,
                                           activePointDistance: activePointDistance,
                                           myPositionDistance: -1)
    return .routePreview(data)
  }

  static func makeTrackRecordingState(trackData: PlacePageTrackData) -> ElevationProfileState {
    let chartData = makeChartData(elevationProfileData: trackData.elevationProfileData)
      ?? makePlaceholderChartData(elevationProfileData: trackData.elevationProfileData)
    let data = ElevationProfileDisplayData(trackInfo: trackData.trackInfo,
                                           chartData: chartData,
                                           activePointDistance: trackData.activePointDistance,
                                           myPositionDistance: trackData.myPositionDistance)
    return .trackRecording(data)
  }

  private static func makeTrackState(trackData: PlacePageTrackData) -> ElevationProfileState? {
    guard trackData.trackInfo.hasElevationInfo,
          let chartData = makeChartData(elevationProfileData: trackData.elevationProfileData) else {
      return nil
    }
    let data = ElevationProfileDisplayData(trackInfo: trackData.trackInfo,
                                           chartData: chartData,
                                           activePointDistance: trackData.activePointDistance,
                                           myPositionDistance: trackData.myPositionDistance)
    return .track(data)
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

  private static func makePlaceholderChartData(elevationProfileData: ElevationProfileData?) -> ElevationProfileChartData {
    let altitude = elevationProfileData?.points.first?.altitude ?? currentAltitude ?? 0
    return .placeholder(altitude: altitude)
  }

  private static var currentAltitude: Double? {
    guard let location = LocationManager.lastLocation(), location.verticalAccuracy >= 0 else {
      return nil
    }
    return location.altitude
  }
}
