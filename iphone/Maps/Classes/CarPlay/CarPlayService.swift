import CarPlay
import Contacts

@objc(MWMCarPlayService)
final class CarPlayService: NSObject {
  @objc static let shared = CarPlayService()
  @objc var isCarplayActivated: Bool = false
  private var searchService: CarPlaySearchService?
  private var router: CarPlayRouter?
  private var window: CPWindow?
  private var interfaceController: CPInterfaceController?
  private var sessionConfiguration: CPSessionConfiguration?
  var currentPositionMode: MWMMyPositionMode = .pendingPosition
  var isSpeedCamActivated: Bool {
    set {
      router?.updateSpeedCameraMode(newValue ? .always: .never)
    }
    get {
      let mode: SpeedCameraManagerMode = router?.speedCameraMode ?? .never
      return mode == .always ? true : false
    }
  }
  var isKeyboardLimited: Bool {
    return sessionConfiguration?.limitedUserInterfaces.contains(.keyboard) ?? false
  }
  private var carplayVC: CarPlayMapViewController? {
    return window?.rootViewController as? CarPlayMapViewController
  }
  private var rootMapTemplate: CPMapTemplate? {
    return interfaceController?.rootTemplate as? CPMapTemplate
  }
  var preparedToPreviewTrips: [CPTrip] = []
  var isUserPanMap: Bool = false

  @objc func setup(window: CPWindow, interfaceController: CPInterfaceController) {
    isCarplayActivated = true
    self.window = window
    self.interfaceController = interfaceController
    self.interfaceController?.delegate = self
    sessionConfiguration = CPSessionConfiguration(delegate: self)
    // Try to use the CarPlay unit's interface style.
    if #available(iOS 13.0, *) {
      if sessionConfiguration?.contentStyle == .light {
        window.overrideUserInterfaceStyle = .light
      } else {
        window.overrideUserInterfaceStyle = .dark
      }
    }
    searchService = CarPlaySearchService()
    let router = CarPlayRouter()
    router.addListener(self)
    router.subscribeToEvents()
    router.setupCarPlaySpeedCameraMode()
    self.router = router
    MWMRouter.unsubscribeFromEvents()
    applyRootViewController()
    if let sessionData = router.restoredNavigationSession() {
      applyNavigationRootTemplate(trip: sessionData.0, routeInfo: sessionData.1)
    } else {
      applyBaseRootTemplate()
      router.restoreTripPreviewOnCarplay(beforeRootTemplateDidAppear: true)
    }
    ThemeManager.invalidate()
    FrameworkHelper.updatePositionArrowOffset(false, offset: 5)
  }

  @objc func destroy() {
    if let carplayVC = carplayVC {
      carplayVC.removeMapView()
    }
    if let mvc = MapViewController.shared() {
      mvc.disableCarPlayRepresentation()
      mvc.remove(self)
    }
    router?.removeListener(self)
    router?.unsubscribeFromEvents()
    router?.setupInitialSpeedCameraMode()
    MWMRouter.subscribeToEvents()
    isCarplayActivated = false
    if router?.currentTrip != nil {
      MWMRouter.showNavigationMapControls()
    } else if router?.previewTrip != nil {
      MWMRouter.rebuild(withBestRouter: true)
    }
    searchService = nil
    router = nil
    sessionConfiguration = nil
    interfaceController = nil
    ThemeManager.invalidate()
    FrameworkHelper.updatePositionArrowOffset(true, offset: 0)
  }

  @objc func interfaceStyle() -> UIUserInterfaceStyle {
    if let window = window,
      window.traitCollection.userInterfaceIdiom == .carPlay {
      return window.traitCollection.userInterfaceStyle
    }
    return .unspecified
  }

  private func applyRootViewController() {
    guard let window = window else { return }
    let carplaySotyboard = UIStoryboard.instance(.carPlay)
    let carplayVC = carplaySotyboard.instantiateInitialViewController() as! CarPlayMapViewController
    window.rootViewController = carplayVC
    if let mapVC = MapViewController.shared() {
      currentPositionMode = mapVC.currentPositionMode
      mapVC.enableCarPlayRepresentation()
      carplayVC.addMapView(mapVC.mapView, mapButtonSafeAreaLayoutGuide: window.mapButtonSafeAreaLayoutGuide)
      mapVC.add(self)
    }
  }

  private func applyBaseRootTemplate() {
    let mapTemplate = MapTemplateBuilder.buildBaseTemplate(positionMode: currentPositionMode)
    mapTemplate.mapDelegate = self
    interfaceController?.setRootTemplate(mapTemplate, animated: true)
    FrameworkHelper.rotateMap(0.0, animated: false)
  }

  private func applyNavigationRootTemplate(trip: CPTrip, routeInfo: RouteInfo) {
    let mapTemplate = MapTemplateBuilder.buildNavigationTemplate()
    mapTemplate.mapDelegate = self
    interfaceController?.setRootTemplate(mapTemplate, animated: true)
    router?.startNavigationSession(forTrip: trip, template: mapTemplate)
    if let estimates = createEstimates(routeInfo: routeInfo) {
      mapTemplate.updateEstimates(estimates, for: trip)
    }

    if let carplayVC = carplayVC {
      carplayVC.updateCurrentSpeed(routeInfo.speedMps, speedLimitMps: routeInfo.speedLimitMps)
      carplayVC.showSpeedControl()
    }
  }

  func pushTemplate(_ templateToPush: CPTemplate, animated: Bool) {
    if let interfaceController = interfaceController {
      switch templateToPush {
      case let list as CPListTemplate:
        list.delegate = self
      case let search as CPSearchTemplate:
        search.delegate = self
      case let map as CPMapTemplate:
        map.mapDelegate = self
      default:
        break
      }
      interfaceController.pushTemplate(templateToPush, animated: animated)
    }
  }

  func popTemplate(animated: Bool) {
    interfaceController?.popTemplate(animated: animated)
  }

  func presentAlert(_ template: CPAlertTemplate, animated: Bool) {
    interfaceController?.dismissTemplate(animated: false)
    interfaceController?.presentTemplate(template, animated: animated)
  }

  func cancelCurrentTrip() {
    router?.cancelTrip()
    if let carplayVC = carplayVC {
      carplayVC.hideSpeedControl()
    }
    updateMapTemplateUIToBase()
  }

  func updateCameraUI(isCameraOnRoute: Bool, speedLimitMps limit: Double?) {
    if let carplayVC = carplayVC {
      carplayVC.updateCameraInfo(isCameraOnRoute: isCameraOnRoute, speedLimitMps: limit)
    }
  }

  func updateMapTemplateUIToBase() {
    guard let mapTemplate = rootMapTemplate else {
        return
    }
    MapTemplateBuilder.configureBaseUI(mapTemplate: mapTemplate)
    if currentPositionMode == .pendingPosition {
      mapTemplate.leadingNavigationBarButtons = []
    } else if currentPositionMode == .follow || currentPositionMode == .followAndRotate {
      MapTemplateBuilder.setupDestinationButton(mapTemplate: mapTemplate)
    } else {
      MapTemplateBuilder.setupRecenterButton(mapTemplate: mapTemplate)
    }
    updateVisibleViewPortState(.default)
    FrameworkHelper.rotateMap(0.0, animated: true)
  }

  func updateMapTemplateUIToTripFinished(_ trip: CPTrip) {
    guard let mapTemplate = rootMapTemplate else {
        return
    }
    mapTemplate.leadingNavigationBarButtons = []
    mapTemplate.trailingNavigationBarButtons = []
    mapTemplate.mapButtons = []
    let doneAction = CPAlertAction(title: L("done"), style: .cancel) { [unowned self] _ in
      self.updateMapTemplateUIToBase()
    }
    var subtitle = ""
    if let locationName = trip.destination.name {
      subtitle = locationName
    }
    if let address = trip.destination.placemark.postalAddress?.street {
      subtitle = subtitle + "\n" + address
    }

    let alert = CPNavigationAlert(titleVariants: [L("trip_finished")],
                                  subtitleVariants: [subtitle],
                                  imageSet: nil,
                                  primaryAction: doneAction,
                                  secondaryAction: nil,
                                  duration: 0)
    mapTemplate.present(navigationAlert: alert, animated: true)
  }

  func updateVisibleViewPortState(_ state: CPViewPortState) {
    guard let carplayVC = carplayVC else {
      return
    }
    carplayVC.updateVisibleViewPortState(state)
  }

  func updateRouteAfterChangingSettings() {
    router?.rebuildRoute()
  }

  @objc func showNoMapAlert() {
    guard let mapTemplate = interfaceController?.topTemplate as? CPMapTemplate,
      let info = mapTemplate.userInfo as? MapInfo,
      info.type == CPConstants.TemplateType.main else {
      return
    }
    let alert = CPAlertTemplate(titleVariants: [L("download_map_carplay")], actions: [])
    alert.userInfo = [CPConstants.TemplateKey.alert: CPConstants.TemplateType.downloadMap]
    presentAlert(alert, animated: true)
  }

  @objc func hideNoMapAlert() {
    if let presentedTemplate = interfaceController?.presentedTemplate,
      let info = presentedTemplate.userInfo as? [String: String],
      let alertType = info[CPConstants.TemplateKey.alert],
      alertType == CPConstants.TemplateType.downloadMap {
      interfaceController?.dismissTemplate(animated: true)
    }
  }
}

// MARK: - CPInterfaceControllerDelegate implementation
extension CarPlayService: CPInterfaceControllerDelegate {
  func templateWillAppear(_ aTemplate: CPTemplate, animated: Bool) {
    guard let info = aTemplate.userInfo as? MapInfo else {
        return
    }
    switch info.type {
    case CPConstants.TemplateType.main:
      updateVisibleViewPortState(.default)
    case CPConstants.TemplateType.preview:
      updateVisibleViewPortState(.preview)
    case CPConstants.TemplateType.navigation:
      updateVisibleViewPortState(.navigation)
    case CPConstants.TemplateType.previewSettings:
      aTemplate.userInfo = MapInfo(type: CPConstants.TemplateType.preview)
    default:
      break
    }
  }

  func templateDidAppear(_ aTemplate: CPTemplate, animated: Bool) {
    guard let mapTemplate = aTemplate as? CPMapTemplate,
      let info = aTemplate.userInfo as? MapInfo else {
        return
    }
    if !preparedToPreviewTrips.isEmpty && info.type == CPConstants.TemplateType.main {
      preparePreview(trips: preparedToPreviewTrips)
      preparedToPreviewTrips = []
      return
    }

    if info.type == CPConstants.TemplateType.preview, let trips = info.trips {
      showPreview(mapTemplate: mapTemplate, trips: trips)
    }
  }

  func templateWillDisappear(_ aTemplate: CPTemplate, animated: Bool) {
    guard let info = aTemplate.userInfo as? MapInfo else {
        return
    }
    if info.type == CPConstants.TemplateType.preview {
      router?.completeRouteAndRemovePoints()
    }
  }

  func templateDidDisappear(_ aTemplate: CPTemplate, animated: Bool) {
    guard !preparedToPreviewTrips.isEmpty,
      let info = aTemplate.userInfo as? [String: String],
      let alertType = info[CPConstants.TemplateKey.alert],
      alertType == CPConstants.TemplateType.redirectRoute ||
        alertType == CPConstants.TemplateType.restoreRoute else {
        return
    }
    preparePreview(trips: preparedToPreviewTrips)
    preparedToPreviewTrips = []
  }
}

// MARK: - CPSessionConfigurationDelegate implementation
extension CarPlayService: CPSessionConfigurationDelegate {
  func sessionConfiguration(_ sessionConfiguration: CPSessionConfiguration,
                            limitedUserInterfacesChanged limitedUserInterfaces: CPLimitableUserInterface) {

  }
  @available(iOS 13.0, *)
  func sessionConfiguration(_ sessionConfiguration: CPSessionConfiguration,
                            contentStyleChanged contentStyle: CPContentStyle) {
    window?.overrideUserInterfaceStyle = contentStyle == .light ? .light : .dark
  }
}

// MARK: - CPMapTemplateDelegate implementation
extension CarPlayService: CPMapTemplateDelegate {
  public func mapTemplateDidShowPanningInterface(_ mapTemplate: CPMapTemplate) {
    isUserPanMap = false
    MapTemplateBuilder.configurePanUI(mapTemplate: mapTemplate)
    FrameworkHelper.stopLocationFollow()
  }

  public func mapTemplateDidDismissPanningInterface(_ mapTemplate: CPMapTemplate) {
    if let info = mapTemplate.userInfo as? MapInfo,
      info.type == CPConstants.TemplateType.navigation {
      MapTemplateBuilder.configureNavigationUI(mapTemplate: mapTemplate)
    } else {
      MapTemplateBuilder.configureBaseUI(mapTemplate: mapTemplate)
    }
    FrameworkHelper.switchMyPositionMode()
  }

  func mapTemplate(_ mapTemplate: CPMapTemplate, panEndedWith direction: CPMapTemplate.PanDirection) {
    var offset = UIOffset(horizontal: 0.0, vertical: 0.0)
    let offsetStep: CGFloat = 0.25
    if direction.contains(.up) { offset.vertical -= offsetStep }
    if direction.contains(.down) { offset.vertical += offsetStep }
    if direction.contains(.left) { offset.horizontal += offsetStep }
    if direction.contains(.right) { offset.horizontal -= offsetStep }
    FrameworkHelper.moveMap(offset)
    isUserPanMap = true
  }

  func mapTemplate(_ mapTemplate: CPMapTemplate, panWith direction: CPMapTemplate.PanDirection) {
    var offset = UIOffset(horizontal: 0.0, vertical: 0.0)
    let offsetStep: CGFloat = 0.1
    if direction.contains(.up) { offset.vertical -= offsetStep }
    if direction.contains(.down) { offset.vertical += offsetStep }
    if direction.contains(.left) { offset.horizontal += offsetStep }
    if direction.contains(.right) { offset.horizontal -= offsetStep }
    FrameworkHelper.moveMap(offset)
    isUserPanMap = true
  }

  func mapTemplate(_ mapTemplate: CPMapTemplate, startedTrip trip: CPTrip, using routeChoice: CPRouteChoice) {
    guard let info = routeChoice.userInfo as? RouteInfo else {
      if let info = routeChoice.userInfo as? [String: Any],
        let code = info[CPConstants.Trip.errorCode] as? RouterResultCode,
        let countries = info[CPConstants.Trip.missedCountries] as? [String] {
        showErrorAlert(code: code, countries: countries)
      }
      return
    }
    mapTemplate.userInfo = MapInfo(type: CPConstants.TemplateType.previewAccepted)
    mapTemplate.hideTripPreviews()

    guard let router = router,
      let interfaceController = interfaceController,
      let rootMapTemplate = rootMapTemplate else {
        return
    }

    MapTemplateBuilder.configureNavigationUI(mapTemplate: rootMapTemplate)

    if interfaceController.templates.count > 1 {
      interfaceController.popToRootTemplate(animated: false)
    }
    router.startNavigationSession(forTrip: trip, template: rootMapTemplate)
    router.startRoute()
    if let estimates = createEstimates(routeInfo: info) {
      rootMapTemplate.updateEstimates(estimates, for: trip)
    }

    if let carplayVC = carplayVC {
      carplayVC.updateCurrentSpeed(info.speedMps, speedLimitMps: info.speedLimitMps)
      carplayVC.showSpeedControl()
    }
    updateVisibleViewPortState(.navigation)
  }

  func mapTemplate(_ mapTemplate: CPMapTemplate, displayStyleFor maneuver: CPManeuver) -> CPManeuverDisplayStyle {
    if let type = maneuver.userInfo as? String,
      type == CPConstants.Maneuvers.secondary {
      return .trailingSymbol
    }
    return .leadingSymbol
  }

  func mapTemplate(_ mapTemplate: CPMapTemplate,
                   selectedPreviewFor trip: CPTrip,
                   using routeChoice: CPRouteChoice) {
    guard let previewTrip = router?.previewTrip, previewTrip == trip else {
      applyUndefinedEstimates(template: mapTemplate, trip: trip)
      router?.buildRoute(trip: trip)
      return
    }
    guard let info = routeChoice.userInfo as? RouteInfo,
      let estimates = createEstimates(routeInfo: info) else {
      applyUndefinedEstimates(template: mapTemplate, trip: trip)
      router?.rebuildRoute()
      return
    }
    mapTemplate.updateEstimates(estimates, for: trip)
    routeChoice.userInfo = nil
    router?.rebuildRoute()
  }
}


// MARK: - CPListTemplateDelegate implementation
extension CarPlayService: CPListTemplateDelegate {
  func listTemplate(_ listTemplate: CPListTemplate, didSelect item: CPListItem, completionHandler: @escaping () -> Void) {
    if let userInfo = item.userInfo as? ListItemInfo {
      switch userInfo.type {
      case CPConstants.ListItemType.history:
        let locale = window?.textInputMode?.primaryLanguage ?? "en"
        guard let searchService = searchService else {
          completionHandler()
          return
        }
        searchService.searchText(item.text ?? "", forInputLocale: locale, completionHandler: { [weak self] results in
          guard let self = self else { return }
          let template = ListTemplateBuilder.buildListTemplate(for: .searchResults(results: results))
          completionHandler()
          self.pushTemplate(template, animated: true)
        })
      case CPConstants.ListItemType.bookmarkLists where userInfo.metadata is CategoryInfo:
        let metadata = userInfo.metadata as! CategoryInfo
        let template = ListTemplateBuilder.buildListTemplate(for: .bookmarks(category: metadata.category))
        completionHandler()
        pushTemplate(template, animated: true)
      case CPConstants.ListItemType.bookmarks where userInfo.metadata is BookmarkInfo:
        let metadata = userInfo.metadata as! BookmarkInfo
        let bookmark = MWMCarPlayBookmarkObject(bookmarkId: metadata.bookmarkId)
        preparePreview(forBookmark: bookmark)
        completionHandler()
      case CPConstants.ListItemType.searchResults where userInfo.metadata is SearchResultInfo:
        let metadata = userInfo.metadata as! SearchResultInfo
        preparePreviewForSearchResults(selectedRow: metadata.originalRow)
        completionHandler()
      default:
        completionHandler()
      }
    }
  }
}

// MARK: - CPSearchTemplateDelegate implementation
extension CarPlayService: CPSearchTemplateDelegate {
  func searchTemplate(_ searchTemplate: CPSearchTemplate, updatedSearchText searchText: String, completionHandler: @escaping ([CPListItem]) -> Void) {
    let locale = window?.textInputMode?.primaryLanguage ?? "en"
    guard let searchService = searchService else {
      completionHandler([])
      return
    }
    searchService.searchText(searchText, forInputLocale: locale, completionHandler: { results in
      var items = [CPListItem]()
      for object in results {
        let item = CPListItem(text: object.title, detailText: object.address)
        item.userInfo = ListItemInfo(type: CPConstants.ListItemType.searchResults,
                                     metadata: SearchResultInfo(originalRow: object.originalRow))
        items.append(item)
      }
      completionHandler(items)
    })
  }

  func searchTemplate(_ searchTemplate: CPSearchTemplate, selectedResult item: CPListItem, completionHandler: @escaping () -> Void) {
    searchService?.saveLastQuery()
    if let info = item.userInfo as? ListItemInfo,
      let metadata = info.metadata as? SearchResultInfo {
      preparePreviewForSearchResults(selectedRow: metadata.originalRow)
    }
    completionHandler()
  }
}

// MARK: - CarPlayRouterListener implementation
extension CarPlayService: CarPlayRouterListener {
  func didCreateRoute(routeInfo: RouteInfo, trip: CPTrip) {
    guard let currentTemplate = interfaceController?.topTemplate as? CPMapTemplate,
      let info = currentTemplate.userInfo as? MapInfo,
      info.type == CPConstants.TemplateType.preview else {
        return
    }
    if let estimates = createEstimates(routeInfo: routeInfo) {
      currentTemplate.updateEstimates(estimates, for: trip)
    }
  }

  func didUpdateRouteInfo(_ routeInfo: RouteInfo, forTrip trip: CPTrip) {
    if let carplayVC = carplayVC {
      carplayVC.updateCurrentSpeed(routeInfo.speedMps, speedLimitMps: routeInfo.speedLimitMps)
    }
    guard let router = router,
      let template = rootMapTemplate else {
        return
    }
    router.updateEstimates()
    if let estimates = createEstimates(routeInfo: routeInfo) {
      template.updateEstimates(estimates, for: trip)
    }
    trip.routeChoices.first?.userInfo = routeInfo
  }

  func didFailureBuildRoute(forTrip trip: CPTrip, code: RouterResultCode, countries: [String]) {
    guard let template = interfaceController?.topTemplate as? CPMapTemplate else { return }
    trip.routeChoices.first?.userInfo = [CPConstants.Trip.errorCode: code, CPConstants.Trip.missedCountries: countries]
    applyUndefinedEstimates(template: template, trip: trip)
  }

  func routeDidFinish(_ trip: CPTrip) {
    if router?.currentTrip == nil { return }
    router?.finishTrip()
    if let carplayVC = carplayVC {
      carplayVC.hideSpeedControl()
    }
    updateMapTemplateUIToTripFinished(trip)
  }
}

// MARK: - LocationModeListener implementation
extension CarPlayService: LocationModeListener {
  func processMyPositionStateModeEvent(_ mode: MWMMyPositionMode) {
    currentPositionMode = mode
    guard let rootMapTemplate = rootMapTemplate,
      let info = rootMapTemplate.userInfo as? MapInfo,
      info.type == CPConstants.TemplateType.main else {
        return
    }
    switch mode {
    case .follow, .followAndRotate:
      if !rootMapTemplate.isPanningInterfaceVisible {
        MapTemplateBuilder.setupDestinationButton(mapTemplate: rootMapTemplate)
      }
    case .notFollow:
      if !rootMapTemplate.isPanningInterfaceVisible {
        MapTemplateBuilder.setupRecenterButton(mapTemplate: rootMapTemplate)
      }
    case .pendingPosition, .notFollowNoPosition:
      rootMapTemplate.leadingNavigationBarButtons = []
    }
  }

  func processMyPositionPendingTimeout() {
  }
}

// MARK: - Alerts and Trip Previews
extension CarPlayService {
  func preparePreviewForSearchResults(selectedRow row: Int) {
    var results = searchService?.lastResults ?? []
    if let currentItemIndex = results.firstIndex(where: { $0.originalRow == row }) {
      let item = results.remove(at: currentItemIndex)
      results.insert(item, at: 0)
    } else {
      results.insert(MWMCarPlaySearchResultObject(forRow: row), at: 0)
    }
    if let router = router,
      let startPoint = MWMRoutePoint(lastLocationAndType: .start,
                                     intermediateIndex: 0) {
      let endPoints = results.compactMap({ MWMRoutePoint(cgPoint: $0.mercatorPoint,
                                                         title: $0.title,
                                                         subtitle: $0.address,
                                                         type: .finish,
                                                         intermediateIndex: 0) })
      let trips = endPoints.map({ router.createTrip(startPoint: startPoint, endPoint: $0) })
      if router.currentTrip == nil {
        preparePreview(trips: trips)
      } else {
        showRerouteAlert(trips: trips)
      }
    }
  }

  func preparePreview(forBookmark bookmark: MWMCarPlayBookmarkObject) {
    if let router = router,
      let startPoint = MWMRoutePoint(lastLocationAndType: .start,
                                      intermediateIndex: 0),
      let endPoint = MWMRoutePoint(cgPoint: bookmark.mercatorPoint,
                                   title: bookmark.prefferedName,
                                   subtitle: bookmark.address,
                                   type: .finish,
                                   intermediateIndex: 0) {
      let trip = router.createTrip(startPoint: startPoint, endPoint: endPoint)
      if router.currentTrip == nil {
        preparePreview(trips: [trip])
      } else {
        showRerouteAlert(trips: [trip])
      }
    }
  }

  func preparePreview(trips: [CPTrip]) {
    let mapTemplate = MapTemplateBuilder.buildTripPreviewTemplate(forTrips: trips)
    if let interfaceController = interfaceController {
      mapTemplate.mapDelegate = self

      if interfaceController.templates.count > 1 {
        interfaceController.popToRootTemplate(animated: false)
      }
      interfaceController.pushTemplate(mapTemplate, animated: false)
    }
  }

  func showPreview(mapTemplate: CPMapTemplate, trips: [CPTrip]) {
    let tripTextConfig = CPTripPreviewTextConfiguration(startButtonTitle: L("trip_start"),
                                                        additionalRoutesButtonTitle: nil,
                                                        overviewButtonTitle: nil)
    mapTemplate.showTripPreviews(trips, textConfiguration: tripTextConfig)
  }

  func createEstimates(routeInfo: RouteInfo) -> CPTravelEstimates? {
    if let distance = Double(routeInfo.targetDistance) {
      let measurement = Measurement(value: distance,
                                    unit: routeInfo.targetUnits)
      let estimates = CPTravelEstimates(distanceRemaining: measurement,
                                        timeRemaining: routeInfo.timeToTarget)
      return estimates
    }
    return nil
  }

  func applyUndefinedEstimates(template: CPMapTemplate, trip: CPTrip) {
    let measurement = Measurement(value: -1,
                                  unit: UnitLength.meters)
    let estimates = CPTravelEstimates(distanceRemaining: measurement,
                                      timeRemaining: -1)
    template.updateEstimates(estimates, for: trip)
  }

  func showRerouteAlert(trips: [CPTrip]) {
    let yesAction = CPAlertAction(title: L("yes"), style: .default, handler: { [unowned self] _ in
      self.router?.cancelTrip()
      self.updateMapTemplateUIToBase()
      self.preparedToPreviewTrips = trips
      self.interfaceController?.dismissTemplate(animated: true)
    })
    let noAction = CPAlertAction(title: L("no"), style: .cancel, handler: { [unowned self] _ in
      self.interfaceController?.dismissTemplate(animated: true)
    })
    let alert = CPAlertTemplate(titleVariants: [L("redirect_route_alert")], actions: [noAction, yesAction])
    alert.userInfo = [CPConstants.TemplateKey.alert: CPConstants.TemplateType.redirectRoute]
    presentAlert(alert, animated: true)
  }

  func showKeyboardAlert() {
    let okAction = CPAlertAction(title: L("ok"), style: .default, handler: { [unowned self] _ in
      self.interfaceController?.dismissTemplate(animated: true)
    })
    let alert = CPAlertTemplate(titleVariants: [L("keyboard_availability_alert")], actions: [okAction])
    presentAlert(alert, animated: true)
  }

  func showErrorAlert(code: RouterResultCode, countries: [String]) {
    var titleVariants = [String]()
    switch code {
    case .noCurrentPosition:
      titleVariants = ["\(L("dialog_routing_check_gps_carplay"))"]
    case .startPointNotFound:
      titleVariants = ["\(L("dialog_routing_change_start_carplay"))"]
    case .endPointNotFound:
      titleVariants = ["\(L("dialog_routing_change_end_carplay"))"]
    case .routeNotFoundRedressRouteError,
         .routeNotFound,
         .inconsistentMWMandRoute:
      titleVariants = ["\(L("dialog_routing_unable_locate_route_carplay"))"]
    case .routeFileNotExist,
         .fileTooOld,
         .needMoreMaps,
         .pointsInDifferentMWM:
      titleVariants = ["\(L("dialog_routing_download_files_carplay"))"]
    case .internalError,
         .intermediatePointNotFound:
      titleVariants = ["\(L("dialog_routing_system_error_carplay"))"]
    case .noError,
         .cancelled,
         .hasWarnings,
         .transitRouteNotFoundNoNetwork,
         .transitRouteNotFoundTooLongPedestrian:
      return
    }

    let okAction = CPAlertAction(title: L("ok"), style: .cancel, handler: { [unowned self] _ in
      self.interfaceController?.dismissTemplate(animated: true)
    })
    let alert = CPAlertTemplate(titleVariants: titleVariants, actions: [okAction])
    presentAlert(alert, animated: true)
  }

  func showRecoverRouteAlert(trip: CPTrip, isTypeCorrect: Bool) {
    let yesAction = CPAlertAction(title: L("ok"), style: .default, handler: { [unowned self] _ in
      var info = trip.userInfo as? [String: MWMRoutePoint]

      if let startPoint = MWMRoutePoint(lastLocationAndType: .start,
                                        intermediateIndex: 0) {
        info?[CPConstants.Trip.start] = startPoint
      }
      trip.userInfo = info
      self.preparedToPreviewTrips = [trip]
      self.router?.updateStartPointAndRebuild(trip: trip)
      self.interfaceController?.dismissTemplate(animated: true)
    })
    let noAction = CPAlertAction(title: L("cancel"), style: .cancel, handler: { [unowned self] _ in
      FrameworkHelper.rotateMap(0.0, animated: false)
      self.router?.completeRouteAndRemovePoints()
      self.interfaceController?.dismissTemplate(animated: true)
    })
    let title = isTypeCorrect ? L("dialog_routing_rebuild_from_current_location_carplay") : L("dialog_routing_rebuild_for_vehicle_carplay")
    let alert = CPAlertTemplate(titleVariants: [title], actions: [noAction, yesAction])
    alert.userInfo = [CPConstants.TemplateKey.alert: CPConstants.TemplateType.restoreRoute]
    presentAlert(alert, animated: true)
  }
}
