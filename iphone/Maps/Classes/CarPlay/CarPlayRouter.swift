import CarPlay
import Contacts

@available(iOS 12.0, *)
protocol CarPlayRouterListener: class {
  func didCreateRoute(routeInfo: RouteInfo,
                      trip: CPTrip)
  func didUpdateRouteInfo(_ routeInfo: RouteInfo, forTrip trip: CPTrip)
  func didFailureBuildRoute(forTrip trip: CPTrip, code: RouterResultCode, countries: [String])
  func routeDidFinish(_ trip: CPTrip)
}


@available(iOS 12.0, *)
@objc(MWMCarPlayRouter)
final class CarPlayRouter: NSObject {
  private let listenerContainer: ListenerContainer<CarPlayRouterListener>
  private var routeSession: CPNavigationSession?
  private var initialSpeedCamSettings: SpeedCameraManagerMode
  var currentTrip: CPTrip? {
    return routeSession?.trip
  }
  var previewTrip: CPTrip?
  var speedCameraMode: SpeedCameraManagerMode {
    return RoutingManager.routingManager.speedCameraMode
  }
  
  override init() {
    listenerContainer = ListenerContainer<CarPlayRouterListener>()
    initialSpeedCamSettings = RoutingManager.routingManager.speedCameraMode
    super.init()
  }
  
  func addListener(_ listener: CarPlayRouterListener) {
    listenerContainer.addListener(listener)
  }
  
  func removeListener(_ listener: CarPlayRouterListener) {
    listenerContainer.removeListener(listener)
  }
  
  func subscribeToEvents() {
    RoutingManager.routingManager.add(self)
  }
  
  func unsubscribeFromEvents() {
    RoutingManager.routingManager.remove(self)
  }
  
  func completeRouteAndRemovePoints() {
    let manager = RoutingManager.routingManager
    manager.stopRoutingAndRemoveRoutePoints(true)
    manager.deleteSavedRoutePoints()
    manager.apply(routeType: .vehicle)
    previewTrip = nil
  }
  
  func rebuildRoute() {
    guard let trip = previewTrip else { return }
    do {
      try RoutingManager.routingManager.buildRoute()
    } catch let error as NSError {
      listenerContainer.forEach({
        let code = RouterResultCode(rawValue: UInt(error.code)) ?? .internalError
        $0.didFailureBuildRoute(forTrip: trip, code: code, countries: [])
      })
    }
  }
  
  func buildRoute(trip: CPTrip) {
    completeRouteAndRemovePoints()
    previewTrip = trip
    guard let info = trip.userInfo as? [String: MWMRoutePoint] else {
      listenerContainer.forEach({
        $0.didFailureBuildRoute(forTrip: trip, code: .routeNotFound, countries: [])
      })
      return
    }
    guard let startPoint = info[CPConstants.Trip.start],
      let endPoint = info[CPConstants.Trip.end] else {
        listenerContainer.forEach({
          var code: RouterResultCode!
          if info[CPConstants.Trip.end] == nil {
            code = .endPointNotFound
          } else {
            code = .startPointNotFound
          }
          $0.didFailureBuildRoute(forTrip: trip, code: code, countries: [])
        })
        return
    }
    
    let manager = RoutingManager.routingManager
    manager.add(routePoint: startPoint)
    manager.add(routePoint: endPoint)
    
    do {
      try manager.buildRoute()
    } catch let error as NSError {
      listenerContainer.forEach({
        let code = RouterResultCode(rawValue: UInt(error.code)) ?? .internalError
        $0.didFailureBuildRoute(forTrip: trip, code: code, countries: [])
      })
    }
  }
  
  func updateStartPointAndRebuild(trip: CPTrip) {
    let manager = RoutingManager.routingManager
    previewTrip = trip
    guard let info = trip.userInfo as? [String: MWMRoutePoint] else {
      listenerContainer.forEach({
        $0.didFailureBuildRoute(forTrip: trip, code: .routeNotFound, countries: [])
      })
      return
    }
    guard let startPoint = info[CPConstants.Trip.start] else {
        listenerContainer.forEach({
          $0.didFailureBuildRoute(forTrip: trip, code: .startPointNotFound, countries: [])
        })
        return
    }
    manager.add(routePoint: startPoint)
    manager.apply(routeType: .vehicle)
    do {
      try manager.buildRoute()
    } catch let error as NSError {
      listenerContainer.forEach({
        let code = RouterResultCode(rawValue: UInt(error.code)) ?? .internalError
        $0.didFailureBuildRoute(forTrip: trip, code: code, countries: [])
      })
    }
  }
  
  func startRoute() {
    let manager = RoutingManager.routingManager
    manager.startRoute()
    Statistics.logEvent(kStatRoutingRouteStart, withParameters: [kStatMode : kStatCarplay])
  }
  
  func setupCarPlaySpeedCameraMode() {
    if case .auto = initialSpeedCamSettings {
      RoutingManager.routingManager.speedCameraMode = .always
    }
  }
  
  func setupInitialSpeedCameraMode() {
    RoutingManager.routingManager.speedCameraMode = initialSpeedCamSettings
  }
  
  func updateSpeedCameraMode(_ mode: SpeedCameraManagerMode) {
    initialSpeedCamSettings = mode
    RoutingManager.routingManager.speedCameraMode = mode
  }
  
  func restoreTripPreviewOnCarplay(beforeRootTemplateDidAppear: Bool) {
    guard MWMRouter.isRestoreProcessCompleted() else {
      DispatchQueue.main.async { [weak self] in
        self?.restoreTripPreviewOnCarplay(beforeRootTemplateDidAppear: false)
      }
      return
    }
    let manager = RoutingManager.routingManager
    MWMRouter.hideNavigationMapControls()
    guard manager.isRoutingActive,
      let startPoint = manager.startPoint,
      let endPoint = manager.endPoint else {
        completeRouteAndRemovePoints()
        return
    }
    let trip = createTrip(startPoint: startPoint,
                          endPoint: endPoint,
                          routeInfo: manager.routeInfo)
    previewTrip = trip
    if manager.type != .vehicle {
      CarPlayService.shared.showRecoverRouteAlert(trip: trip, isTypeCorrect: false)
      return
    }
    if !startPoint.isMyPosition {
      CarPlayService.shared.showRecoverRouteAlert(trip: trip, isTypeCorrect: true)
      return
    }
    if beforeRootTemplateDidAppear {
      CarPlayService.shared.preparedToPreviewTrips = [trip]
    } else {
      CarPlayService.shared.preparePreview(trips: [trip])
    }
  }
  
  func restoredNavigationSession() -> (CPTrip, RouteInfo)? {
    let manager = RoutingManager.routingManager
    if manager.isOnRoute,
      manager.type == .vehicle,
      let startPoint = manager.startPoint,
      let endPoint = manager.endPoint,
      let routeInfo = manager.routeInfo {
      MWMRouter.hideNavigationMapControls()
      let trip = createTrip(startPoint: startPoint,
                            endPoint: endPoint,
                            routeInfo: routeInfo)
      previewTrip = trip
      return (trip, routeInfo)
    }
    return nil
  }
}

// MARK: - Navigation session management
@available(iOS 12.0, *)
extension CarPlayRouter {
  func startNavigationSession(forTrip trip: CPTrip, template: CPMapTemplate) {
    routeSession = template.startNavigationSession(for: trip)
    routeSession?.pauseTrip(for: .loading, description: nil)
    updateUpcomingManeuvers()
    RoutingManager.routingManager.setOnNewTurnCallback { [weak self] in
      self?.updateUpcomingManeuvers()
    }
  }
  
  func cancelTrip() {
    routeSession?.cancelTrip()
    routeSession = nil
    completeRouteAndRemovePoints()
    RoutingManager.routingManager.resetOnNewTurnCallback()
  }
  
  func finishTrip() {
    routeSession?.finishTrip()
    routeSession = nil
    completeRouteAndRemovePoints()
    RoutingManager.routingManager.resetOnNewTurnCallback()
  }
  
  func updateUpcomingManeuvers() {
    let maneuvers = createUpcomingManeuvers()
    routeSession?.upcomingManeuvers = maneuvers
  }

  func updateEstimates() {
    guard let routeSession = routeSession,
          let routeInfo = RoutingManager.routingManager.routeInfo,
          let primaryManeuver = routeSession.upcomingManeuvers.first,
          let estimates = createEstimates(routeInfo) else {
      return
    }
    routeSession.updateEstimates(estimates, for: primaryManeuver)
  }

  private func createEstimates(_ routeInfo: RouteInfo) -> CPTravelEstimates? {
    guard let distance = Double(routeInfo.distanceToTurn) else {
      return nil
    }

    let measurement = Measurement(value: distance, unit: routeInfo.turnUnits)
    return CPTravelEstimates(distanceRemaining: measurement, timeRemaining: 0.0)
  }
  
  private func createUpcomingManeuvers() -> [CPManeuver] {
    guard let routeInfo = RoutingManager.routingManager.routeInfo else {
      return []
    }
    var maneuvers = [CPManeuver]()
    let primaryManeuver = CPManeuver()
    primaryManeuver.userInfo = CPConstants.Maneuvers.primary
    var instructionVariant = routeInfo.streetName
    if routeInfo.roundExitNumber != 0 {
      let ordinalExitNumber = NumberFormatter.localizedString(from: NSNumber(value: routeInfo.roundExitNumber),
                                                              number: .ordinal)
      let exitNumber = String(coreFormat: L("carplay_roundabout_exit"),
                              arguments: [ordinalExitNumber])
      instructionVariant = instructionVariant.isEmpty ? exitNumber : (exitNumber + ", " + instructionVariant)
    }
    primaryManeuver.instructionVariants = [instructionVariant]
    if let imageName = routeInfo.turnImageName,
      let symbol = UIImage(named: imageName) {
      primaryManeuver.symbolSet = CPImageSet(lightContentImage: symbol,
                                             darkContentImage: symbol)
    }
    if let estimates = createEstimates(routeInfo) {
      primaryManeuver.initialTravelEstimates = estimates
    }
    maneuvers.append(primaryManeuver)
    if let imageName = routeInfo.nextTurnImageName,
      let symbol = UIImage(named: imageName) {
      let secondaryManeuver = CPManeuver()
      secondaryManeuver.userInfo = CPConstants.Maneuvers.secondary
      secondaryManeuver.instructionVariants = [L("then_turn")]
      secondaryManeuver.symbolSet = CPImageSet(lightContentImage: symbol,
                                               darkContentImage: symbol)
      maneuvers.append(secondaryManeuver)
    }
    return maneuvers
  }
  
  func createTrip(startPoint: MWMRoutePoint, endPoint: MWMRoutePoint, routeInfo: RouteInfo? = nil) -> CPTrip {
    let startPlacemark = MKPlacemark(coordinate: CLLocationCoordinate2D(latitude: startPoint.latitude,
                                                                        longitude: startPoint.longitude))
    let endPlacemark = MKPlacemark(coordinate: CLLocationCoordinate2D(latitude: endPoint.latitude,
                                                                      longitude: endPoint.longitude),
                                   addressDictionary: [CNPostalAddressStreetKey: endPoint.subtitle ?? ""])
    let startItem = MKMapItem(placemark: startPlacemark)
    let endItem = MKMapItem(placemark: endPlacemark)
    endItem.name = endPoint.title
    
    let routeChoice = CPRouteChoice(summaryVariants: [" "], additionalInformationVariants: [], selectionSummaryVariants: [])
    routeChoice.userInfo = routeInfo
    
    let trip = CPTrip(origin: startItem, destination: endItem, routeChoices: [routeChoice])
    trip.userInfo = [CPConstants.Trip.start: startPoint, CPConstants.Trip.end: endPoint]
    return trip
  }
}

// MARK: - RoutingManagerListener implementation
@available(iOS 12.0, *)
extension CarPlayRouter: RoutingManagerListener {
  func updateCameraInfo(isCameraOnRoute: Bool, speedLimit limit: String?) {
    CarPlayService.shared.updateCameraUI(isCameraOnRoute: isCameraOnRoute, speedLimit: limit)
  }
  
  func processRouteBuilderEvent(with code: RouterResultCode, countries: [String]) {
    guard let trip = previewTrip else {
      return
    }
    switch code {
    case .noError, .hasWarnings:
      let manager = RoutingManager.routingManager
      if manager.isRouteFinished {
        listenerContainer.forEach({
          $0.routeDidFinish(trip)
        })
        return
      }
      if let info = manager.routeInfo {
        previewTrip?.routeChoices.first?.userInfo = info
        if routeSession == nil {
          listenerContainer.forEach({
            $0.didCreateRoute(routeInfo: info,
                              trip: trip)
          })
        } else {
          listenerContainer.forEach({
            $0.didUpdateRouteInfo(info, forTrip: trip)
          })
          updateUpcomingManeuvers()
        }
      }
    default:
      listenerContainer.forEach({
        $0.didFailureBuildRoute(forTrip: trip, code: code, countries: countries)
      })
    }
  }
  
  func didLocationUpdate(_ notifications: [String]) {
    guard let trip = previewTrip else { return }
    
    let manager = RoutingManager.routingManager
    if manager.isRouteFinished {
      listenerContainer.forEach({
        $0.routeDidFinish(trip)
      })
      return
    }
    
    guard let routeInfo = manager.routeInfo,
      manager.isRoutingActive else { return }
    listenerContainer.forEach({
      $0.didUpdateRouteInfo(routeInfo, forTrip: trip)
    })
    
    let tts = MWMTextToSpeech.tts()!
    if manager.isOnRoute && tts.active {
      tts.playTurnNotifications(notifications)
      tts.playWarningSound()
    }
  }
}
