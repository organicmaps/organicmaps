// swiftformat:disable preferForLoop
import CarPlay
import Contacts

protocol CarPlayRouterListener: AnyObject {
  func didCreateRoute(routeInfo: RouteInfo,
                      trip: CPTrip)
  func didUpdateRouteInfo(_ routeInfo: RouteInfo, forTrip trip: CPTrip)
  func didFailureBuildRoute(forTrip trip: CPTrip, code: RouterResultCode, countries: [String])
  func routeDidFinish(_ trip: CPTrip)
}

@objc(MWMCarPlayRouter)
final class CarPlayRouter: NSObject {
  private static let initialManeuverRepublishDelay: TimeInterval = 0.5

  private let listenerContainer: ListenerContainer<CarPlayRouterListener>
  private var routeSession: CPNavigationSession?
  private var registeredManeuvers = [CPManeuver]()
  private var initialSpeedCamSettings: SpeedCameraManagerMode
  private var isRoutingPresentationActive = false
  var currentTrip: CPTrip? {
    routeSession?.trip
  }

  var previewTrip: CPTrip?
  var speedCameraMode: SpeedCameraManagerMode {
    RoutingManager.routingManager.speedCameraMode
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

  func setRoutingPresentationActive(_ isActive: Bool) {
    isRoutingPresentationActive = isActive
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
      listenerContainer.forEach {
        let code = RouterResultCode(rawValue: UInt(error.code)) ?? .internalError
        $0.didFailureBuildRoute(forTrip: trip, code: code, countries: [])
      }
    }
  }

  func buildRoute(trip: CPTrip) {
    completeRouteAndRemovePoints()
    previewTrip = trip
    guard let info = trip.userInfo as? [String: MWMRoutePoint] else {
      listenerContainer.forEach {
        $0.didFailureBuildRoute(forTrip: trip, code: .routeNotFound, countries: [])
      }
      return
    }
    guard let startPoint = info[CPConstants.Trip.start],
          let endPoint = info[CPConstants.Trip.end]
    else {
      listenerContainer.forEach {
        var code: RouterResultCode!
        if info[CPConstants.Trip.end] == nil {
          code = .endPointNotFound
        } else {
          code = .startPointNotFound
        }
        $0.didFailureBuildRoute(forTrip: trip, code: code, countries: [])
      }
      return
    }

    let manager = RoutingManager.routingManager
    manager.add(routePoint: startPoint)
    manager.add(routePoint: endPoint)

    do {
      try manager.buildRoute()
    } catch let error as NSError {
      listenerContainer.forEach {
        let code = RouterResultCode(rawValue: UInt(error.code)) ?? .internalError
        $0.didFailureBuildRoute(forTrip: trip, code: code, countries: [])
      }
    }
  }

  func updateStartPointAndRebuild(trip: CPTrip) {
    let manager = RoutingManager.routingManager
    previewTrip = trip
    guard let info = trip.userInfo as? [String: MWMRoutePoint] else {
      listenerContainer.forEach {
        $0.didFailureBuildRoute(forTrip: trip, code: .routeNotFound, countries: [])
      }
      return
    }
    guard let startPoint = info[CPConstants.Trip.start] else {
      listenerContainer.forEach {
        $0.didFailureBuildRoute(forTrip: trip, code: .startPointNotFound, countries: [])
      }
      return
    }
    manager.add(routePoint: startPoint)
    manager.apply(routeType: .vehicle)
    do {
      try manager.buildRoute()
    } catch let error as NSError {
      listenerContainer.forEach {
        let code = RouterResultCode(rawValue: UInt(error.code)) ?? .internalError
        $0.didFailureBuildRoute(forTrip: trip, code: code, countries: [])
      }
    }
  }

  func startRoute() {
    let manager = RoutingManager.routingManager
    manager.startRoute()
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
          let endPoint = manager.endPoint
    else {
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

extension CarPlayRouter {
  func startNavigationSession(forTrip trip: CPTrip, template: CPMapTemplate, isRestoring: Bool = false) {
    guard routeSession == nil else {
      let errorMessage = "Route session is already running."
      LOG(.error, errorMessage)
      Toast.show(withText: errorMessage, alignment: .top)
      return
    }
    LOG(.info, "Starting a new navigation session")
    routeSession = template.startNavigationSession(for: trip)
    resetNavigationMetadata()
    if isRestoring {
      scheduleInitialManeuverRepublish()
    }
    RoutingManager.routingManager.setOnNewTurnCallback { [weak self] in
      self?.advanceToNextManeuver()
    }
  }

  func cancelNavigationSession() {
    LOG(.info, "Сancelling navigation session")
    routeSession?.cancelTrip()
    routeSession = nil
    registeredManeuvers.removeAll()
    RoutingManager.routingManager.resetOnNewTurnCallback()
  }

  func cancelTrip() {
    LOG(.info, "Сancelling trip")
    cancelNavigationSession()
    completeRouteAndRemovePoints()
  }

  func finishTrip() {
    LOG(.info, "Finishing trip")
    routeSession?.finishTrip()
    routeSession = nil
    registeredManeuvers.removeAll()
    completeRouteAndRemovePoints()
    RoutingManager.routingManager.resetOnNewTurnCallback()
  }

  func updateUpcomingManeuvers(reusingPrimaryManeuver primaryManeuver: CPManeuver? = nil) {
    guard let routeSession,
          let routeInfo = RoutingManager.routingManager.routeInfo
    else {
      return
    }

    let maneuvers = createUpcomingManeuvers(routeInfo: routeInfo, primaryManeuver: primaryManeuver)
    if #available(iOS 17.4, *) {
      let newManeuvers = primaryManeuver == nil ? maneuvers : Array(maneuvers.dropFirst())
      if !newManeuvers.isEmpty {
        routeSession.add(newManeuvers)
      }
    }
    registeredManeuvers = maneuvers
    routeSession.upcomingManeuvers = maneuvers
    if #available(iOS 17.4, *) {
      updateCurrentNavigationMetadata(routeSession: routeSession, routeInfo: routeInfo)
    }
  }

  func updateEstimates(_ routeInfo: RouteInfo) {
    guard let routeSession = routeSession,
          let primaryManeuver = registeredManeuvers.first
    else {
      return
    }
    routeSession.updateEstimates(createEstimates(routeInfo), for: primaryManeuver)
    if #available(iOS 17.4, *) {
      updateCurrentNavigationMetadata(routeSession: routeSession, routeInfo: routeInfo)
    }
  }

  private func createEstimates(_ routeInfo: RouteInfo) -> CPTravelEstimates {
    let measurement = Measurement(value: routeInfo.distanceToTurn, unit: routeInfo.turnUnits)
    return CPTravelEstimates(distanceRemaining: measurement, timeRemaining: 0.0)
  }

  @available(iOS 17.4, *)
  private func replaceNavigationMetadata(routeInfo: RouteInfo) {
    guard let routeSession else { return }

    let maneuvers = createUpcomingManeuvers(routeInfo: routeInfo)

    let tripEstimates = CPTravelEstimates(
      distanceRemaining: Measurement(value: routeInfo.targetDistance, unit: routeInfo.targetUnits),
      timeRemaining: routeInfo.timeToTarget
    )
    // CPRouteInformation requires a current lane guidance even when the app has no lane data.
    let emptyLaneGuidance = CPLaneGuidance()
    emptyLaneGuidance.lanes = []
    emptyLaneGuidance.instructionVariants = maneuvers[0].instructionVariants
    let routeInformation = CPRouteInformation(
      maneuvers: maneuvers,
      laneGuidances: [],
      currentManeuvers: maneuvers,
      currentLaneGuidance: emptyLaneGuidance,
      trip: tripEstimates,
      maneuverTravelEstimates: createEstimates(routeInfo)
    )

    routeSession.pauseTrip(for: .rerouting, description: nil)
    routeSession.resumeTrip(updatedRouteInformation: routeInformation)
    registeredManeuvers = maneuvers
    routeSession.upcomingManeuvers = maneuvers
    updateCurrentNavigationMetadata(routeSession: routeSession, routeInfo: routeInfo)
  }

  private func resetNavigationMetadata() {
    registeredManeuvers.removeAll()
    updateUpcomingManeuvers()
  }

  private func advanceToNextManeuver() {
    // Reuse the already registered secondary maneuver when it becomes primary to avoid adding it twice.
    let nextManeuver = registeredManeuvers.count > 1 ? registeredManeuvers[1] : nil
    updateUpcomingManeuvers(reusingPrimaryManeuver: nextManeuver)
  }

  private func scheduleInitialManeuverRepublish() {
    guard let routeSession else { return }
    // Even after the root template is installed, a restored session's initial maneuvers
    // may not be picked up by the instrument cluster. Republish them once after session setup.
    DispatchQueue.main.asyncAfter(deadline: .now() + Self.initialManeuverRepublishDelay) { [weak self, weak routeSession] in
      guard let self, let routeSession,
            self.routeSession === routeSession,
            !self.registeredManeuvers.isEmpty
      else {
        return
      }
      routeSession.upcomingManeuvers = self.registeredManeuvers
    }
  }

  @available(iOS 17.4, *)
  private func updateCurrentNavigationMetadata(routeSession: CPNavigationSession, routeInfo: RouteInfo) {
    routeSession.currentRoadNameVariants = routeInfo.currentStreetName.isEmpty ? [] : [routeInfo.currentStreetName]

    let distanceMeters = Measurement(value: routeInfo.distanceToTurn, unit: routeInfo.turnUnits)
      .converted(to: .meters).value
    routeSession.maneuverState = CarPlayManeuverMapper.maneuverState(distanceToTurnMeters: distanceMeters)
  }

  private func createUpcomingManeuvers(routeInfo: RouteInfo,
                                       primaryManeuver: CPManeuver? = nil) -> [CPManeuver] {
    let primaryManeuver = primaryManeuver ?? CPManeuver()
    primaryManeuver.userInfo = CPConstants.Maneuvers.primary
    var instructionVariant = routeInfo.streetName
    if routeInfo.roundExitNumber != 0 {
      let ordinalExitNumber = NumberFormatter.localizedString(from: NSNumber(value: routeInfo.roundExitNumber),
                                                              number: .ordinal)
      let exitNumber = String(format: L("carplay_roundabout_exit"),
                              arguments: [ordinalExitNumber])
      instructionVariant = instructionVariant.isEmpty ? exitNumber : (exitNumber + ", " + instructionVariant)
    }
    primaryManeuver.instructionVariants = [instructionVariant]
    primaryManeuver.symbolImage = nil
    primaryManeuver.dashboardSymbolImage = nil
    if let imageName = routeInfo.turnImageName,
       let symbol = UIImage(named: imageName) {
      primaryManeuver.symbolImage = symbol.withRenderingMode(.alwaysOriginal)
      primaryManeuver.dashboardSymbolImage = symbol.withRenderingMode(.alwaysTemplate)
    }
    primaryManeuver.initialTravelEstimates = createEstimates(routeInfo)
    if #available(iOS 17.4, *) {
      CarPlayManeuverMapper.configure(primaryManeuver,
                                      direction: routeInfo.turnDirection,
                                      roadName: routeInfo.streetName,
                                      roundaboutExitNumber: routeInfo.roundExitNumber)
      primaryManeuver.trafficSide = routeInfo.isLeftHandTraffic ? .left : .right
    }

    guard let imageName = routeInfo.nextTurnImageName,
          let symbol = UIImage(named: imageName)
    else {
      return [primaryManeuver]
    }

    let secondaryManeuver = CPManeuver()
    secondaryManeuver.userInfo = CPConstants.Maneuvers.secondary
    secondaryManeuver.instructionVariants = [L("then_turn")]
    secondaryManeuver.symbolImage = symbol.withRenderingMode(.alwaysOriginal) // always white on green
    secondaryManeuver.dashboardSymbolImage = symbol.withRenderingMode(.alwaysTemplate) // black/white on transparent
    if #available(iOS 17.4, *) {
      CarPlayManeuverMapper.configure(secondaryManeuver,
                                      direction: routeInfo.nextTurnDirection,
                                      roadName: routeInfo.nextStreetName)
      secondaryManeuver.trafficSide = routeInfo.isLeftHandTraffic ? .left : .right
    }
    return [primaryManeuver, secondaryManeuver]
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

extension CarPlayRouter: RoutingManagerListener {
  func updateCameraInfo(isCameraOnRoute: Bool, speedLimitMps limit: Double) {
    CarPlayService.shared.updateCameraUI(isCameraOnRoute: isCameraOnRoute, speedLimitMps: limit < 0 ? nil : limit)
  }

  func processRouteBuilderEvent(with code: RouterResultCode, countries: [String]) {
    guard isRoutingPresentationActive else { return }
    guard let trip = previewTrip else {
      return
    }
    switch code {
    case .noError, .hasWarnings:
      let manager = RoutingManager.routingManager
      if manager.isRouteFinished {
        listenerContainer.forEach {
          $0.routeDidFinish(trip)
        }
        return
      }
      // Building a route disables following in the core. Resume it when an active CarPlay
      // navigation session requested the rebuild (for example, after changing route options).
      if routeSession != nil, !manager.isOnRoute {
        manager.startRoute()
      }
      if let info = manager.routeInfo {
        previewTrip?.routeChoices.first?.userInfo = info
        if routeSession == nil {
          listenerContainer.forEach {
            $0.didCreateRoute(routeInfo: info,
                              trip: trip)
          }
        } else {
          if #available(iOS 17.4, *) {
            replaceNavigationMetadata(routeInfo: info)
          } else {
            resetNavigationMetadata()
          }
          listenerContainer.forEach {
            $0.didUpdateRouteInfo(info, forTrip: trip)
          }
        }
      }
    default:
      listenerContainer.forEach {
        $0.didFailureBuildRoute(forTrip: trip, code: code, countries: countries)
      }
    }
  }

  func didLocationUpdate(_ notifications: [String]) {
    guard isRoutingPresentationActive else { return }
    guard let trip = previewTrip else { return }

    let manager = RoutingManager.routingManager
    if manager.isRouteFinished {
      listenerContainer.forEach {
        $0.routeDidFinish(trip)
      }
      return
    }

    guard let routeInfo = manager.routeInfo,
          manager.isRoutingActive else { return }
    listenerContainer.forEach {
      $0.didUpdateRouteInfo(routeInfo, forTrip: trip)
    }

    let tts = MWMTextToSpeech.tts()!
    if manager.isOnRoute, tts.active {
      tts.playTurnNotifications(notifications)
      tts.playWarningSound()
    }
  }
}
