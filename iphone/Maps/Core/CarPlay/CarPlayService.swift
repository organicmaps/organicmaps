import CarPlay
import Contacts

private enum RoutingOwner {
  case device
  case carPlay
}

private struct CarPlaySpeedState {
  var currentSpeedMps: Double = 0
  var speedLimitMps: Double?
  var isCameraOnRoute = false
  var cameraSpeedLimitMps: Double?
  var isVisible = false
}

@objc(MWMCarPlayService)
final class CarPlayService: NSObject {
  private enum Constants {
    static let openCarPlayURL = URL(string: "om://carplay")
  }

  @objc static let shared = CarPlayService()

  @objc var isCarplayActivated: Bool {
    effectiveDisplay != .device
  }

  private var searchService: CarPlaySearchService?
  private var router: CarPlayRouter?
  private var window: CPWindow?
  private var dashboardWindow: UIWindow?
  private weak var dashboardScene: CPTemplateApplicationDashboardScene?
  private var dashboardController: CPDashboardController?
  private var interfaceController: CPInterfaceController?
  private var sessionConfiguration: CPSessionConfiguration?
  private var mapState = CarPlayMapOwnershipState()
  // Layout can run while the shared map is moving, before attachedDisplay is updated.
  private var transitioningTo: CarPlayMapDisplay?
  private var routingOwner: RoutingOwner?
  private var isCarPlayRouterSubscribed = false
  private var needsCarPlayRoutingRestore = false
  private var isPhoneSceneConnected = false
  private var activeCarPlayDisplay: CarPlayMapDisplay?
  private var phoneModeAlert: CPAlertTemplate?
  private var isTemplateOperationInProgress = false
  private weak var visibleCarPlayTemplate: CPTemplate?
  private var currentViewPortState: CPViewPortState = .default
  private var speedState = CarPlaySpeedState()

  private var currentPositionMode: MWMMyPositionMode {
    MapViewController.shared()?.currentPositionMode ?? .pendingPosition
  }

  var isSpeedCamActivated: Bool {
    set {
      router?.updateSpeedCameraMode(newValue ? .always : .never)
    }
    get {
      let mode: SpeedCameraManagerMode = router?.speedCameraMode ?? .never
      return mode == .always ? true : false
    }
  }

  var isKeyboardLimited: Bool {
    sessionConfiguration?.limitedUserInterfaces.contains(.keyboard) ?? false
  }

  private var carplayVC: CarPlayMapViewController? {
    window?.rootViewController as? CarPlayMapViewController
  }

  private var dashboardVC: CarPlayDashboardMapViewController? {
    dashboardWindow?.rootViewController as? CarPlayDashboardMapViewController
  }

  private var rootMapTemplate: CPMapTemplate? {
    interfaceController?.rootTemplate as? CPMapTemplate
  }

  private var isOnRoute: Bool {
    MWMRouter.isOnRoute()
  }

  @objc var carplayLayoutMargins: UIEdgeInsets {
    switch effectiveDisplay {
    case .device:
      return .zero
    case .mainCarPlay:
      carplayVC?.view.layoutIfNeeded()
      return carplayVC?.view.layoutMargins ?? .zero
    case .dashboardCarPlay:
      dashboardVC?.view.layoutIfNeeded()
      return dashboardVC?.view.safeAreaInsets ?? .zero
    }
  }

  var preparedToPreviewTrips: [CPTrip] = []
  private var searchText = ""

  func setup(window: CPWindow, interfaceController: CPInterfaceController) {
    let isNewSession = self.window !== window || self.interfaceController !== interfaceController
    let replacesConnectedSession = isNewSession && self.interfaceController != nil

    if replacesConnectedSession {
      router?.cancelNavigationSession()
    }
    if self.window !== window {
      carplayVC?.removeMapView()
    }
    if self.interfaceController !== interfaceController {
      phoneModeAlert = nil
      isTemplateOperationInProgress = false
      visibleCarPlayTemplate = nil
    }

    self.window = window
    self.interfaceController = interfaceController
    self.interfaceController?.delegate = self

    let configuration = CPSessionConfiguration(delegate: self)
    sessionConfiguration = configuration
    searchService = searchService ?? CarPlaySearchService()
    let router = ensureRouter()

    applyRootViewController()
    // Attach the one shared map even when CarPlay connects before the phone scene creates a window.
    updateMapPlacement()

    updateContentStyle(configuration.contentStyle)
    if isNewSession || rootMapTemplate == nil {
      configureRootTemplate(using: router)
    } else {
      // Reusing a connected CarPlay session does not run setRootTemplate again, so refresh its UI.
      restoreCarPlayTemplateUI()
    }
    updatePhoneModeAlert()
  }

  func setupDashboard(scene: CPTemplateApplicationDashboardScene,
                      window: UIWindow,
                      dashboardController: CPDashboardController) {
    if dashboardWindow !== window {
      dashboardVC?.removeMapView()
    }
    dashboardScene = scene
    dashboardWindow = window
    if !(window.rootViewController is CarPlayDashboardMapViewController) {
      window.rootViewController = CarPlayDashboardMapViewController()
    }
    _ = ensureRouter()
    self.dashboardController = dashboardController
    updateMapPlacement()
    updateMapPlaceholders()
    refreshSpeedStateFromRoutingManager()
    renderSpeedState()
  }

  private func updateDashboardButtons() {
    guard let dashboardController else { return }
    dashboardController.shortcutButtons = DashboardBuilder.buildShortcutButtons(
      isMapOnPhone: mapState.isPhoneSelected,
      openCarPlay: { [weak self] in
        self?.openMainCarPlay()
      }
    )
  }

  private func updateMapPlaceholders() {
    dashboardVC?.setPhoneModePlaceholderVisible(mapState.isPhoneSelected)
    updateDashboardButtons()
  }

  func showOnPhone() {
    mapState.selectPhone()
    updateMapPlacement()
  }

  func openMainCarPlay() {
    activateMainCarPlay()
    guard let dashboardScene, let url = Constants.openCarPlayURL else { return }
    dashboardScene.open(url, options: nil)
  }

  func handleOpenCarPlayURL(_ url: URL) -> Bool {
    guard let openCarPlayURL = Constants.openCarPlayURL, url == openCarPlayURL else { return false }
    activateMainCarPlay()
    return true
  }

  func sceneDidBecomeActive(_ scene: UIScene) {
    handleCarPlaySceneActivation(scene)
  }

  func sceneWillEnterForeground(_ scene: UIScene) {
    handleCarPlaySceneActivation(scene)
  }

  private func handleCarPlaySceneActivation(_ scene: UIScene) {
    if scene is CPTemplateApplicationDashboardScene {
      activeCarPlayDisplay = .dashboardCarPlay
    } else if scene is CPTemplateApplicationScene {
      // CarPlay opens Main when the Dashboard map widget is tapped without invoking our Dashboard controls.
      // Treat that scene activation as an explicit request to move the shared map back to the car.
      activateMainCarPlay()
      return
    } else {
      return
    }
    updateMapPlacement()
  }

  func phoneSceneDidConnect() {
    isPhoneSceneConnected = true
    updateMapPlacement()
  }

  func phoneSceneDidDisconnect() {
    isPhoneSceneConnected = false
    mapState.phoneDidDisconnect()
    updateMapPlacement()
  }

  private func activateMainCarPlay() {
    mapState.selectCar()
    activeCarPlayDisplay = .mainCarPlay
    updateMapPlacement()
  }

  private func updateMapPlacement() {
    guard let destination = mapState.desiredDisplay(for: mapAvailability) else { return }
    guard mapState.attachedDisplay != destination || !isMapAttached(to: destination) else { return }
    moveMap(to: destination)
  }

  private func moveMap(to destination: CarPlayMapDisplay) {
    guard let mapVC = MapViewController.shared() else {
      return
    }
    mapVC.loadViewIfNeeded()
    guard isAvailable(destination) else {
      return
    }

    let mapWasOnPhone = mapVC.mapView.superview === mapVC.view
    transitioningTo = destination
    defer { transitioningTo = nil }

    // Detach from both CarPlay containers first. Phone detachment is handled by
    // enableCarPlayRepresentation because it also installs the existing phone placeholder.
    carplayVC?.removeMapView()
    dashboardVC?.removeMapView()
    mapVC.remove(self)

    switch destination {
    case .device:
      if !mapWasOnPhone {
        mapVC.disableCarPlayRepresentation()
      }
    case .mainCarPlay:
      guard let window, let carplayVC else {
        return
      }
      if mapWasOnPhone {
        mapVC.enableCarPlayRepresentation()
      }
      carplayVC.addMapView(mapVC.mapView, mapButtonSafeAreaLayoutGuide: window.mapButtonSafeAreaLayoutGuide)
      mapVC.add(self)
    case .dashboardCarPlay:
      guard let dashboardVC else {
        return
      }
      if mapWasOnPhone {
        mapVC.enableCarPlayRepresentation()
      }
      dashboardVC.addMapView(mapVC.mapView)
      mapVC.add(self)
    }

    mapState.didAttach(to: destination)
    updateMapPlaceholders()
    updatePresentation(for: destination)
    assertSingleMapOwner(mapVC)
  }

  private func updatePresentation(for destination: CarPlayMapDisplay) {
    updateRoutingPresentation(for: destination)

    let toDevice = destination == .device
    FrameworkHelper.updatePositionArrowOffset(toDevice, offset: toDevice ? 0 : 5)
    if toDevice {
      CarPlayWindowScaleAdjuster.restorePhoneScale()
    } else if let destinationWindow = window(for: destination) {
      CarPlayWindowScaleAdjuster.applyCarPlayScale(destinationWindow)
    }

    switch destination {
    case .device:
      break
    case .mainCarPlay:
      carplayVC?.view.layoutIfNeeded()
      // CPInterfaceController owns the template stack. Moving the shared map must not rebuild it:
      // doing so while Main CarPlay appears can race the transition and drop its buttons.
      updateVisibleViewPortState(currentViewPortState)
    case .dashboardCarPlay:
      dashboardVC?.view.layoutIfNeeded()
      dashboardVC?.updateVisibleViewport()
    }

    if destination != .device {
      refreshSpeedStateFromRoutingManager()
    }
    renderSpeedState()
    ThemeManager.invalidate()
    updatePhoneModeAlert()
  }

  private func updateRoutingPresentation(for display: CarPlayMapDisplay) {
    let destination: RoutingOwner = display == .device ? .device : .carPlay
    guard routingOwner != destination, let router else { return }

    switch destination {
    case .device:
      let hadCurrentTrip = router.currentTrip != nil
      let hadPreviewTrip = router.previewTrip != nil
      router.setRoutingPresentationActive(false)
      router.removeListener(self)
      router.setupInitialSpeedCameraMode()
      MWMRouter.subscribeToEvents()
      router.cancelNavigationSession()
      needsCarPlayRoutingRestore = false
      if hadCurrentTrip {
        MWMRouter.showNavigationMapControls()
      } else if hadPreviewTrip {
        MWMRouter.rebuild(withBestRouter: true)
      }
    case .carPlay:
      if !isCarPlayRouterSubscribed {
        router.subscribeToEvents()
        isCarPlayRouterSubscribed = true
      }
      router.addListener(self)
      router.setRoutingPresentationActive(true)
      router.setupCarPlaySpeedCameraMode()
      MWMRouter.unsubscribeFromEvents()
      MWMRouter.hideNavigationMapControls()
      needsCarPlayRoutingRestore = routingOwner == .device
    }

    routingOwner = destination
  }

  private func restoreCarPlayTemplateUI() {
    guard let mapTemplate = rootMapTemplate else {
      return
    }

    mapTemplate.mapDelegate = self
    mapTemplate.tripEstimateStyle = rootTemplateStyle
    if let info = mapTemplate.userInfo as? MapInfo {
      switch info.type {
      case CPConstants.TemplateType.navigation:
        MapTemplateBuilder.configureNavigationUI(mapTemplate, positionMode: currentPositionMode)
      case CPConstants.TemplateType.main:
        MapTemplateBuilder.configureBaseUI(
          mapTemplate,
          positionMode: currentPositionMode,
          isOnRoute: isOnRoute
        )
      default:
        break
      }
    }
    processMyPositionStateModeEvent(currentPositionMode)
  }

  private func restoreCarPlayRoutingStateIfNeeded() {
    guard needsCarPlayRoutingRestore,
          mapState.attachedDisplay != .device,
          let router,
          let mapTemplate = rootMapTemplate
    else {
      return
    }

    needsCarPlayRoutingRestore = false
    mapTemplate.mapDelegate = self
    mapTemplate.tripEstimateStyle = rootTemplateStyle

    if let (trip, routeInfo) = router.restoredNavigationSession() {
      MapTemplateBuilder.configureNavigationUI(mapTemplate, positionMode: currentPositionMode)
      router.startNavigationSession(forTrip: trip, template: mapTemplate)
      if let estimates = createEstimates(routeInfo: routeInfo) {
        mapTemplate.updateEstimates(estimates, for: trip)
      }
      updateSpeedState(routeInfo: routeInfo, isVisible: true)
      updateVisibleViewPortState(.navigation)
      return
    }

    MapTemplateBuilder.configureBaseUI(
      mapTemplate,
      positionMode: currentPositionMode,
      isOnRoute: isOnRoute
    )
    hideSpeedState()
    updateVisibleViewPortState(.default)
    FrameworkHelper.rotateMap(0.0, animated: false)
    router.restoreTripPreviewOnCarplay(beforeRootTemplateDidAppear: false)
  }

  private func configureRootTemplate(using router: CarPlayRouter) {
    // This path restores the routing UI itself; do not repeat it from the root-template completion.
    needsCarPlayRoutingRestore = false
    guard mapState.attachedDisplay != .device else {
      applyBaseRootTemplate()
      return
    }

    if let sessionData = router.restoredNavigationSession() {
      applyNavigationRootTemplate(trip: sessionData.0, routeInfo: sessionData.1)
    } else {
      applyBaseRootTemplate()
      router.restoreTripPreviewOnCarplay(beforeRootTemplateDidAppear: true)
    }
  }

  private func ensureRouter() -> CarPlayRouter {
    if let router {
      return router
    }

    let router = CarPlayRouter()
    // Camera show/clear callbacks keep the cached CarPlay UI state exact while the phone owns routing.
    // Other route and TTS processing remains disabled until routingOwner switches to CarPlay.
    router.subscribeToEvents()
    isCarPlayRouterSubscribed = true
    self.router = router
    return router
  }

  private func updatePhoneModeAlert() {
    guard let interfaceController else { return }
    let shouldPresent = mapState.isPhoneSelected

    if shouldPresent, rootMapTemplate == nil {
      return
    }

    if isTemplateOperationInProgress {
      return
    }

    if shouldPresent {
      if let phoneModeAlert, interfaceController.presentedTemplate === phoneModeAlert {
        return
      }
      if interfaceController.presentedTemplate != nil {
        dismissPresentedTemplate(on: interfaceController)
        return
      }
      presentPhoneModeAlert(on: interfaceController)
      return
    }

    guard let phoneModeAlert,
          interfaceController.presentedTemplate === phoneModeAlert
    else {
      phoneModeAlert = nil
      restoreCarPlayRoutingStateIfNeeded()
      return
    }
    dismissPresentedTemplate(on: interfaceController)
  }

  private func presentPhoneModeAlert(on interfaceController: CPInterfaceController) {
    let switchToCarAction = CPAlertAction(
      title: L("car_continue_in_the_car"),
      style: .default,
      handler: { [weak self] _ in
        self?.activateMainCarPlay()
      }
    )
    let alert = CPAlertTemplate(
      titleVariants: [L("car_used_on_the_phone_screen")],
      actions: [switchToCarAction]
    )
    phoneModeAlert = alert
    isTemplateOperationInProgress = true

    interfaceController.presentTemplate(alert, animated: false) { [weak self, weak interfaceController] success, error in
      guard let self else { return }
      guard self.interfaceController === interfaceController else { return }
      isTemplateOperationInProgress = false
      templateCompletion(success, error)
      if !success {
        phoneModeAlert = nil
        return
      }
      updatePhoneModeAlert()
    }
  }

  private func dismissPresentedTemplate(on interfaceController: CPInterfaceController) {
    isTemplateOperationInProgress = true
    interfaceController.dismissTemplate(animated: false) { [weak self, weak interfaceController] success, error in
      guard let self else { return }
      guard self.interfaceController === interfaceController else { return }
      isTemplateOperationInProgress = false
      templateCompletion(success, error)
      guard success else { return }
      phoneModeAlert = nil
      updatePhoneModeAlert()
    }
  }

  func destroyDashboard(window disconnectedWindow: UIWindow) {
    guard dashboardWindow === disconnectedWindow else {
      return
    }

    dashboardVC?.removeMapView()
    dashboardScene = nil
    dashboardController = nil
    dashboardWindow = nil
    if activeCarPlayDisplay == .dashboardCarPlay {
      activeCarPlayDisplay = nil
    }
    updateMapPlacement()
    teardownRouterIfCarPlayDisconnected()
  }

  func destroy(window disconnectedWindow: CPWindow) {
    guard window === disconnectedWindow else {
      return
    }

    carplayVC?.removeMapView()
    window = nil
    interfaceController = nil
    sessionConfiguration = nil
    searchService = nil
    phoneModeAlert = nil
    isTemplateOperationInProgress = false
    visibleCarPlayTemplate = nil
    if activeCarPlayDisplay == .mainCarPlay {
      activeCarPlayDisplay = nil
    }

    updateMapPlacement()

    if dashboardWindow != nil {
      router?.cancelNavigationSession()
    }
    teardownRouterIfCarPlayDisconnected()
  }

  func destroy() {
    guard let window else { return }
    destroy(window: window)
  }

  private func teardownRouterIfCarPlayDisconnected() {
    guard window == nil, dashboardWindow == nil, let router else { return }
    // moveMap() may not run on disconnect, so hand routing back here too. No-op if already phone-owned.
    updateRoutingPresentation(for: .device)
    if isCarPlayRouterSubscribed {
      router.removeListener(self)
      router.unsubscribeFromEvents()
      isCarPlayRouterSubscribed = false
    }
    router.cancelNavigationSession()
    self.router = nil
    routingOwner = .device
    needsCarPlayRoutingRestore = false
    mapState.reset()
    speedState = CarPlaySpeedState()
  }

  private var mapAvailability: CarPlayMapAvailability {
    CarPlayMapAvailability(
      isDeviceConnected: isPhoneSceneConnected && MapViewController.shared() != nil,
      isMainCarPlayConnected: window != nil && interfaceController != nil && carplayVC != nil,
      isDashboardConnected: dashboardWindow != nil && dashboardVC != nil,
      isMainCarPlayVisible: visibleCarPlayTemplate != nil,
      activeCarPlayDisplay: activeCarPlayDisplay
    )
  }

  private func isAvailable(_ display: CarPlayMapDisplay) -> Bool {
    mapAvailability.contains(display)
  }

  private func isMapAttached(to display: CarPlayMapDisplay) -> Bool {
    guard let mapVC = MapViewController.shared() else { return false }
    switch display {
    case .device:
      return mapVC.mapView.superview === mapVC.view
    case .mainCarPlay:
      return carplayVC?.mapView === mapVC.mapView
    case .dashboardCarPlay:
      return dashboardVC?.mapView === mapVC.mapView
    }
  }

  private func window(for display: CarPlayMapDisplay) -> UIWindow? {
    switch display {
    case .device:
      return MapsAppDelegate.theApp().window
    case .mainCarPlay:
      return window
    case .dashboardCarPlay:
      return dashboardWindow
    }
  }

  private var effectiveDisplay: CarPlayMapDisplay {
    transitioningTo ?? mapState.attachedDisplay
  }

  private func applyRootViewController() {
    guard let window else { return }
    if !(window.rootViewController is CarPlayMapViewController) {
      window.rootViewController = UIStoryboard.instance(.carPlay).instantiateInitialViewController()
    }
  }

  private func assertSingleMapOwner(_ mapVC: MapViewController) {
    let owners = [
      mapVC.mapView.superview === mapVC.view,
      carplayVC?.mapView === mapVC.mapView,
      dashboardVC?.mapView === mapVC.mapView,
    ].filter { $0 }.count
    assert(owners == 1, "The shared map must have exactly one owner, got \(owners)")
  }

  @objc func interfaceStyle() -> UIUserInterfaceStyle {
    if effectiveDisplay == .dashboardCarPlay,
       dashboardWindow?.traitCollection.userInterfaceIdiom == .carPlay {
      return dashboardWindow?.traitCollection.userInterfaceStyle ?? .unspecified
    }
    if let window = window,
       window.traitCollection.userInterfaceIdiom == .carPlay {
      return rootTemplateStyle == .dark ? .dark : .light
    }
    return .unspecified
  }

  private func updateContentStyle(_ contentStyle: CPContentStyle) {
    rootTemplateStyle = contentStyle == .dark ? .dark : .light
    // Update the current map style in accordance with the CarPlay content theme.
    ThemeManager.invalidate()
  }

  private var rootTemplateStyle: CPTripEstimateStyle = .light {
    didSet {
      (interfaceController?.rootTemplate as? CPMapTemplate)?.tripEstimateStyle = rootTemplateStyle
    }
  }

  private func applyBaseRootTemplate() {
    let mapTemplate = MapTemplateBuilder.buildBaseTemplate(positionMode: currentPositionMode, isOnRoute: isOnRoute)
    mapTemplate.mapDelegate = self
    mapTemplate.tripEstimateStyle = rootTemplateStyle
    guard let interfaceController else { return }
    isTemplateOperationInProgress = true
    interfaceController.setRootTemplate(mapTemplate, animated: true) { [weak self, weak interfaceController] success, error in
      guard let self else { return }
      guard self.interfaceController === interfaceController else { return }
      isTemplateOperationInProgress = false
      templateCompletion(success, error)
      if success {
        restoreCarPlayTemplateUI()
        updatePhoneModeAlert()
      }
    }
    FrameworkHelper.rotateMap(0.0, animated: false)
  }

  private func applyNavigationRootTemplate(trip: CPTrip, routeInfo: RouteInfo) {
    let mapTemplate = MapTemplateBuilder.buildNavigationTemplate(positionMode: currentPositionMode)
    mapTemplate.mapDelegate = self
    guard let interfaceController else { return }
    isTemplateOperationInProgress = true
    interfaceController.setRootTemplate(mapTemplate, animated: true) { [weak self, weak interfaceController] success, error in
      guard let self else { return }
      guard self.interfaceController === interfaceController else { return }
      isTemplateOperationInProgress = false
      templateCompletion(success, error)
      if success {
        restoreCarPlayTemplateUI()
        updatePhoneModeAlert()
      }
    }
    router?.startNavigationSession(forTrip: trip, template: mapTemplate)
    if let estimates = createEstimates(routeInfo: routeInfo) {
      mapTemplate.tripEstimateStyle = rootTemplateStyle
      mapTemplate.updateEstimates(estimates, for: trip)
    }

    updateSpeedState(routeInfo: routeInfo, isVisible: true)
  }

  func pushTemplate(_ templateToPush: CPTemplate, animated: Bool) {
    if let interfaceController = interfaceController {
      switch templateToPush {
      case let search as CPSearchTemplate:
        search.delegate = self
      case let map as CPMapTemplate:
        map.mapDelegate = self
      default:
        break
      }
      interfaceController.pushTemplate(templateToPush, animated: animated, completion: templateCompletion)
    }
  }

  func popTemplate(animated: Bool) {
    interfaceController?.popTemplate(animated: animated, completion: templateCompletion)
  }

  func presentAlert(_ template: CPAlertTemplate, animated: Bool) {
    guard let interfaceController else { return }
    let present = { [weak self, weak interfaceController] in
      guard let self, let interfaceController,
            self.interfaceController === interfaceController
      else {
        return
      }
      interfaceController.presentTemplate(template, animated: animated, completion: templateCompletion)
    }
    guard interfaceController.presentedTemplate != nil else {
      present()
      return
    }
    interfaceController.dismissTemplate(animated: false) { [weak self] success, error in
      self?.templateCompletion(success, error)
      guard success else { return }
      present()
    }
  }

  func cancelCurrentTrip() {
    LOG(.info, "Cancel current trip")
    router?.cancelTrip()
    hideSpeedState()
    updateMapTemplateUIToBase()
  }

  func updateCameraUI(isCameraOnRoute: Bool, speedLimitMps limit: Double?) {
    speedState.isCameraOnRoute = isCameraOnRoute
    speedState.cameraSpeedLimitMps = limit
    renderSpeedState()
  }

  private func updateSpeedState(routeInfo: RouteInfo, isVisible: Bool? = nil) {
    speedState.currentSpeedMps = routeInfo.speedMps
    speedState.speedLimitMps = routeInfo.speedLimitMps
    if let isVisible {
      speedState.isVisible = isVisible
    }
    renderSpeedState()
  }

  private func hideSpeedState() {
    speedState.isVisible = false
    speedState.isCameraOnRoute = false
    speedState.cameraSpeedLimitMps = nil
    renderSpeedState()
  }

  private func refreshSpeedStateFromRoutingManager() {
    guard isOnRoute, let routeInfo = RoutingManager.routingManager.routeInfo else {
      speedState.isVisible = false
      return
    }
    speedState.currentSpeedMps = routeInfo.speedMps
    speedState.speedLimitMps = routeInfo.speedLimitMps
    speedState.isVisible = true
  }

  private func renderSpeedState() {
    carplayVC?.updateCurrentSpeed(
      speedState.currentSpeedMps,
      speedLimitMps: speedState.speedLimitMps
    )
    carplayVC?.updateCameraInfo(
      isCameraOnRoute: speedState.isCameraOnRoute,
      speedLimitMps: speedState.cameraSpeedLimitMps
    )
    dashboardVC?.updateCurrentSpeed(
      speedState.currentSpeedMps,
      speedLimitMps: speedState.speedLimitMps
    )
    dashboardVC?.updateCameraInfo(
      isCameraOnRoute: speedState.isCameraOnRoute,
      speedLimitMps: speedState.cameraSpeedLimitMps
    )

    let showOnMainCarPlay = speedState.isVisible && mapState.attachedDisplay == .mainCarPlay
    let showOnDashboard = speedState.isVisible && mapState.attachedDisplay == .dashboardCarPlay
    if showOnMainCarPlay {
      carplayVC?.showSpeedControl()
    } else {
      carplayVC?.hideSpeedControl()
    }
    if showOnDashboard {
      dashboardVC?.showSpeedControl()
    } else {
      dashboardVC?.hideSpeedControl()
    }
  }

  func updateMapTemplateUIToBase() {
    guard let mapTemplate = rootMapTemplate else {
      return
    }
    MapTemplateBuilder.configureBaseUI(mapTemplate, positionMode: currentPositionMode, isOnRoute: isOnRoute)
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
      updateMapTemplateUIToBase()
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
                                  image: nil,
                                  primaryAction: doneAction,
                                  secondaryAction: nil,
                                  duration: 0)
    mapTemplate.present(navigationAlert: alert, animated: true)
  }

  func updateVisibleViewPortState(_ state: CPViewPortState) {
    currentViewPortState = state
    if mapState.attachedDisplay == .dashboardCarPlay {
      dashboardVC?.updateVisibleViewport()
    } else if mapState.attachedDisplay == .mainCarPlay {
      carplayVC?.updateVisibleViewPortState(state)
    }
  }

  func updateRouteAfterChangingSettings() {
    router?.rebuildRoute()
  }

  @objc func showNoMapAlert() {
    guard let mapTemplate = interfaceController?.topTemplate as? CPMapTemplate,
          let info = mapTemplate.userInfo as? MapInfo,
          info.type == CPConstants.TemplateType.main
    else {
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
      interfaceController?.dismissTemplate(animated: true, completion: templateCompletion)
    }
  }

  private func templateCompletion(_: Bool, _ error: Error?) {
    guard let error else { return }
    LOG(.warning, "CarPlay template operation failed with error: \(error.localizedDescription)")
  }
}

// MARK: - CPInterfaceControllerDelegate

extension CarPlayService: CPInterfaceControllerDelegate {
  func templateWillAppear(_ aTemplate: CPTemplate, animated _: Bool) {
    visibleCarPlayTemplate = aTemplate
    updateMapPlacement()
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

  func templateDidAppear(_ aTemplate: CPTemplate, animated _: Bool) {
    guard let mapTemplate = aTemplate as? CPMapTemplate,
          let info = aTemplate.userInfo as? MapInfo
    else {
      return
    }
    if !preparedToPreviewTrips.isEmpty, info.type == CPConstants.TemplateType.main {
      preparePreview(trips: preparedToPreviewTrips)
      preparedToPreviewTrips = []
      return
    }

    if info.type == CPConstants.TemplateType.preview, let trips = info.trips {
      showPreview(mapTemplate: mapTemplate, trips: trips)
    }
  }

  func templateWillDisappear(_ aTemplate: CPTemplate, animated _: Bool) {
    guard let info = aTemplate.userInfo as? MapInfo else {
      return
    }
    if info.type == CPConstants.TemplateType.preview {
      // The alert only covers the preview: the route now belongs to the phone and must survive.
      guard !mapState.isPhoneSelected else { return }
      router?.completeRouteAndRemovePoints()
    }
  }

  func templateDidDisappear(_ aTemplate: CPTemplate, animated _: Bool) {
    if visibleCarPlayTemplate === aTemplate {
      visibleCarPlayTemplate = nil
      // A replacement template normally starts appearing before the previous one finishes
      // disappearing. Defer one run-loop turn so an in-app template transition keeps the map on
      // MainCP. The template visibility callbacks drive automatic MainCP/Dashboard ownership; this
      // deferred reconciliation handles the case where the main template actually leaves the screen.
      DispatchQueue.main.async { [weak self] in
        self?.updateMapPlacement()
      }
    }
    guard !preparedToPreviewTrips.isEmpty,
          let info = aTemplate.userInfo as? [String: String],
          let alertType = info[CPConstants.TemplateKey.alert],
          alertType == CPConstants.TemplateType.redirectRoute ||
          alertType == CPConstants.TemplateType.restoreRoute
    else {
      return
    }
    preparePreview(trips: preparedToPreviewTrips)
    preparedToPreviewTrips = []
  }
}

// MARK: - CPSessionConfigurationDelegate

extension CarPlayService: CPSessionConfigurationDelegate {
  func sessionConfiguration(_: CPSessionConfiguration,
                            limitedUserInterfacesChanged _: CPLimitableUserInterface) {}

  func sessionConfiguration(_: CPSessionConfiguration,
                            contentStyleChanged contentStyle: CPContentStyle) {
    // Handle the CarPlay content style changing triggered by the 'Always Show Dark Maps' toggle.
    updateContentStyle(contentStyle)
  }
}

// MARK: - CPMapTemplateDelegate

extension CarPlayService: CPMapTemplateDelegate {
  func mapTemplateDidShowPanningInterface(_ mapTemplate: CPMapTemplate) {
    MapTemplateBuilder.configurePanUI(mapTemplate)
    FrameworkHelper.stopLocationFollow()
  }

  func mapTemplateDidDismissPanningInterface(_ mapTemplate: CPMapTemplate) {
    if let info = mapTemplate.userInfo as? MapInfo,
       info.type == CPConstants.TemplateType.navigation {
      MapTemplateBuilder.configureNavigationUI(mapTemplate, positionMode: currentPositionMode)
    } else {
      MapTemplateBuilder.configureBaseUI(mapTemplate, positionMode: currentPositionMode, isOnRoute: isOnRoute)
    }
    FrameworkHelper.switchMyPositionMode()
  }

  func mapTemplate(_: CPMapTemplate, panEndedWith direction: CPMapTemplate.PanDirection) {
    var offset = UIOffset(horizontal: 0.0, vertical: 0.0)
    let offsetStep: CGFloat = 0.25
    if direction.contains(.up) {
      offset.vertical -= offsetStep
    }
    if direction.contains(.down) {
      offset.vertical += offsetStep
    }
    if direction.contains(.left) {
      offset.horizontal += offsetStep
    }
    if direction.contains(.right) {
      offset.horizontal -= offsetStep
    }
    FrameworkHelper.moveMap(offset)
  }

  func mapTemplate(_: CPMapTemplate, panWith direction: CPMapTemplate.PanDirection) {
    var offset = UIOffset(horizontal: 0.0, vertical: 0.0)
    let offsetStep: CGFloat = 0.1
    if direction.contains(.up) {
      offset.vertical -= offsetStep
    }
    if direction.contains(.down) {
      offset.vertical += offsetStep
    }
    if direction.contains(.left) {
      offset.horizontal += offsetStep
    }
    if direction.contains(.right) {
      offset.horizontal -= offsetStep
    }
    FrameworkHelper.moveMap(offset)
  }

  func mapTemplate(_ mapTemplate: CPMapTemplate, didUpdatePanGestureWithTranslation translation: CGPoint, velocity _: CGPoint) {
    let scaleFactor = carplayVC?.mapView?.contentScaleFactor ?? 1
    mapTemplate.hidesButtonsWithNavigationBar = false
    FrameworkHelper.scrollMap(toDistanceX: -scaleFactor * translation.x, andY: -scaleFactor * translation.y)
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
          let rootMapTemplate = rootMapTemplate
    else {
      return
    }

    MapTemplateBuilder.configureNavigationUI(rootMapTemplate, positionMode: currentPositionMode)

    if interfaceController.templates.count > 1 {
      interfaceController.popToRootTemplate(animated: false, completion: templateCompletion)
    }
    router.startNavigationSession(forTrip: trip, template: rootMapTemplate)
    router.startRoute()
    if let estimates = createEstimates(routeInfo: info) {
      rootMapTemplate.updateEstimates(estimates, for: trip)
    }

    updateSpeedState(routeInfo: info, isVisible: true)
    updateVisibleViewPortState(.navigation)
  }

  func mapTemplate(_: CPMapTemplate, displayStyleFor maneuver: CPManeuver) -> CPManeuverDisplayStyle {
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
          let estimates = createEstimates(routeInfo: info)
    else {
      applyUndefinedEstimates(template: mapTemplate, trip: trip)
      router?.rebuildRoute()
      return
    }
    mapTemplate.updateEstimates(estimates, for: trip)
    routeChoice.userInfo = nil
    router?.rebuildRoute()
  }
}

// MARK: - CPSearchTemplateDelegate

extension CarPlayService: CPSearchTemplateDelegate {
  func searchTemplate(_: CPSearchTemplate, updatedSearchText searchText: String, completionHandler: @escaping ([CPListItem]) -> Void) {
    self.searchText = searchText
    let locale = window?.textInputMode?.primaryLanguage ?? "en"
    guard let searchService = searchService else {
      completionHandler([])
      return
    }
    searchService.searchText(self.searchText, forInputLocale: locale, completionHandler: { results in
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

  func searchTemplate(_: CPSearchTemplate, selectedResult item: CPListItem, completionHandler: @escaping () -> Void) {
    searchService?.saveLastQuery()
    if let info = item.userInfo as? ListItemInfo,
       let metadata = info.metadata as? SearchResultInfo {
      preparePreviewForSearchResults(selectedRow: metadata.originalRow)
    }
    completionHandler()
  }

  func searchTemplateSearchButtonPressed(_: CPSearchTemplate) {
    let locale = window?.textInputMode?.primaryLanguage ?? "en"
    guard let searchService = searchService else {
      return
    }
    searchService.searchText(searchText, forInputLocale: locale, completionHandler: { [weak self] results in
      guard let self = self else { return }
      let template = ListTemplateBuilder.buildListTemplate(for: .searchResults(results: results))
      self.pushTemplate(template, animated: true)
    })
  }
}

// MARK: - CarPlayRouterListener

extension CarPlayService: CarPlayRouterListener {
  func didCreateRoute(routeInfo: RouteInfo, trip: CPTrip) {
    guard let currentTemplate = interfaceController?.topTemplate as? CPMapTemplate,
          let info = currentTemplate.userInfo as? MapInfo,
          info.type == CPConstants.TemplateType.preview
    else {
      return
    }
    if let estimates = createEstimates(routeInfo: routeInfo) {
      currentTemplate.updateEstimates(estimates, for: trip)
    }
  }

  func didUpdateRouteInfo(_ routeInfo: RouteInfo, forTrip trip: CPTrip) {
    updateSpeedState(routeInfo: routeInfo)
    guard let router = router,
          let template = rootMapTemplate
    else {
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
    if router?.currentTrip == nil {
      return
    }
    router?.finishTrip()
    hideSpeedState()
    updateMapTemplateUIToTripFinished(trip)
  }
}

// MARK: - LocationModeListener implementation

extension CarPlayService: LocationModeListener {
  func processMyPositionStateModeEvent(_ mode: MWMMyPositionMode) {
    guard let rootMapTemplate else { return }
    MapTemplateBuilder.setupMapButtons(rootMapTemplate, positionMode: mode)
    MapTemplateBuilder.setupLeadingNavigationBarButtons(rootMapTemplate, positionMode: mode, isOnRoute: isOnRoute)
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
      let endPoints = results.compactMap { MWMRoutePoint(cgPoint: $0.mercatorPoint,
                                                         title: $0.title,
                                                         subtitle: $0.address,
                                                         type: .finish,
                                                         intermediateIndex: 0) }
      let trips = endPoints.map { router.createTrip(startPoint: startPoint, endPoint: $0) }
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
    guard let interfaceController else { return }
    mapTemplate.mapDelegate = self

    guard interfaceController.templates.count > 1 else {
      pushTripPreview(mapTemplate, on: interfaceController)
      return
    }

    interfaceController.popToRootTemplate(animated: false) { [weak self, weak interfaceController] success, error in
      guard let self, let interfaceController else { return }
      templateCompletion(success, error)
      guard success, self.interfaceController === interfaceController else { return }
      pushTripPreview(mapTemplate, on: interfaceController)
    }
  }

  private func pushTripPreview(_ mapTemplate: CPMapTemplate, on interfaceController: CPInterfaceController) {
    interfaceController.pushTemplate(mapTemplate, animated: false, completion: templateCompletion)
  }

  func showPreview(mapTemplate: CPMapTemplate, trips: [CPTrip]) {
    let tripTextConfig = CPTripPreviewTextConfiguration(startButtonTitle: L("trip_start"),
                                                        additionalRoutesButtonTitle: nil,
                                                        overviewButtonTitle: nil)
    mapTemplate.showTripPreviews(trips, textConfiguration: tripTextConfig)
  }

  func handleListItemSelection(_ selectableItem: CPSelectableListItem, completionHandler: @escaping () -> Void) {
    guard let item = selectableItem as? CPListItem,
          let userInfo = item.userInfo as? ListItemInfo else {
      completionHandler()
      return
    }

    switch userInfo.type {
    case CPConstants.ListItemType.history:
      let locale = window?.textInputMode?.primaryLanguage ?? "en"
      guard let searchService = searchService else {
        completionHandler()
        return
      }
      searchService.searchText(item.text ?? "", forInputLocale: locale, completionHandler: { [weak self] results in
        guard let self else {
          completionHandler()
          return
        }
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

  func createEstimates(routeInfo: RouteInfo) -> CPTravelEstimates? {
    let measurement = Measurement(value: routeInfo.targetDistance, unit: routeInfo.targetUnits)
    return CPTravelEstimates(distanceRemaining: measurement, timeRemaining: routeInfo.timeToTarget)
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
      router?.cancelTrip()
      updateMapTemplateUIToBase()
      preparedToPreviewTrips = trips
      interfaceController?.dismissTemplate(animated: true, completion: templateCompletion)
    })
    let noAction = CPAlertAction(title: L("no"), style: .cancel, handler: { [unowned self] _ in
      interfaceController?.dismissTemplate(animated: true, completion: templateCompletion)
    })
    let alert = CPAlertTemplate(titleVariants: [L("redirect_route_alert")], actions: [noAction, yesAction])
    alert.userInfo = [CPConstants.TemplateKey.alert: CPConstants.TemplateType.redirectRoute]
    presentAlert(alert, animated: true)
  }

  func showKeyboardAlert() {
    let okAction = CPAlertAction(title: L("ok"), style: .default, handler: { [unowned self] _ in
      interfaceController?.dismissTemplate(animated: true, completion: templateCompletion)
    })
    let alert = CPAlertTemplate(titleVariants: [L("keyboard_availability_alert")], actions: [okAction])
    presentAlert(alert, animated: true)
  }

  func showErrorAlert(code: RouterResultCode, countries _: [String]) {
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
      interfaceController?.dismissTemplate(animated: true, completion: templateCompletion)
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
      preparedToPreviewTrips = [trip]
      router?.updateStartPointAndRebuild(trip: trip)
      interfaceController?.dismissTemplate(animated: true, completion: templateCompletion)
    })
    let noAction = CPAlertAction(title: L("cancel"), style: .cancel, handler: { [unowned self] _ in
      FrameworkHelper.rotateMap(0.0, animated: false)
      router?.completeRouteAndRemovePoints()
      interfaceController?.dismissTemplate(animated: true, completion: templateCompletion)
    })
    let title = isTypeCorrect ? L("dialog_routing_rebuild_from_current_location_carplay") : L("dialog_routing_rebuild_for_vehicle_carplay")
    let alert = CPAlertTemplate(titleVariants: [title], actions: [noAction, yesAction])
    alert.userInfo = [CPConstants.TemplateKey.alert: CPConstants.TemplateType.restoreRoute]
    presentAlert(alert, animated: true)
  }
}
