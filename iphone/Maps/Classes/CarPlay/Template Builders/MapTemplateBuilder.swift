import CarPlay

@available(iOS 12.0, *)
final class MapTemplateBuilder {
  enum MapButtonType {
    case startPanning
    case zoomIn
    case zoomOut
  }
  enum BarButtonType {
    case dismissPaning
    case destination
    case recenter
    case settings
    case mute
    case unmute
    case redirectRoute
    case endRoute
  }
  // MARK: - CPMapTemplate builders
  class func buildBaseTemplate(positionMode: MWMMyPositionMode) -> CPMapTemplate {
    let mapTemplate = CPMapTemplate()
    mapTemplate.hidesButtonsWithNavigationBar = false
    configureBaseUI(mapTemplate: mapTemplate)
    if positionMode == .pendingPosition {
      mapTemplate.leadingNavigationBarButtons = []
    } else if positionMode == .follow || positionMode == .followAndRotate {
      setupDestinationButton(mapTemplate: mapTemplate)
    } else {
      setupRecenterButton(mapTemplate: mapTemplate)
    }
    return mapTemplate
  }
  
  class func buildNavigationTemplate() -> CPMapTemplate {
    let mapTemplate = CPMapTemplate()
    mapTemplate.hidesButtonsWithNavigationBar = false
    configureNavigationUI(mapTemplate: mapTemplate)
    return mapTemplate
  }
  
  class func buildTripPreviewTemplate(forTrips trips: [CPTrip]) -> CPMapTemplate {
    let mapTemplate = CPMapTemplate()
    mapTemplate.userInfo = MapInfo(type: CPConstants.TemplateType.preview, trips: trips)
    mapTemplate.mapButtons = []
    mapTemplate.leadingNavigationBarButtons = []
    let settingsButton = buildBarButton(type: .settings) { _ in
      mapTemplate.userInfo = MapInfo(type: CPConstants.TemplateType.previewSettings)
      let gridTemplate = SettingsTemplateBuilder.buildGridTemplate()
      CarPlayService.shared.pushTemplate(gridTemplate, animated: true)
      Statistics.logEvent(kStatCarplaySettingsOpen, withParameters: [kStatFrom : kStatOverview])
    }
    mapTemplate.trailingNavigationBarButtons = [settingsButton]
    return mapTemplate
  }
  
  // MARK: - MapTemplate UI configs
  class func configureBaseUI(mapTemplate: CPMapTemplate) {
    mapTemplate.userInfo = MapInfo(type: CPConstants.TemplateType.main)
    let panningButton = buildMapButton(type: .startPanning) { _ in
      mapTemplate.showPanningInterface(animated: true)
    }
    let zoomInButton = buildMapButton(type: .zoomIn) { _ in
      FrameworkHelper.zoomMap(.in)
      Alohalytics.logEvent(kStatCarplayZoom, with: [kStatIsZoomIn : true,
                                                    kStatIsPanActivated: false])
    }
    let zoomOutButton = buildMapButton(type: .zoomOut) { _ in
      FrameworkHelper.zoomMap(.out)
      Alohalytics.logEvent(kStatCarplayZoom, with: [kStatIsZoomIn : false,
                                                    kStatIsPanActivated: false])
    }
    mapTemplate.mapButtons = [panningButton, zoomInButton, zoomOutButton]
    
    let settingsButton = buildBarButton(type: .settings) { _ in
      let gridTemplate = SettingsTemplateBuilder.buildGridTemplate()
      CarPlayService.shared.pushTemplate(gridTemplate, animated: true)
      Statistics.logEvent(kStatCarplaySettingsOpen, withParameters: [kStatFrom : kStatMap])
    }
    mapTemplate.trailingNavigationBarButtons = [settingsButton]
  }
  
  class func configurePanUI(mapTemplate: CPMapTemplate) {
    let zoomInButton = buildMapButton(type: .zoomIn) { _ in
      FrameworkHelper.zoomMap(.in)
      Alohalytics.logEvent(kStatCarplayZoom, with: [kStatIsZoomIn : true,
                                                    kStatIsPanActivated: true])
    }
    let zoomOutButton = buildMapButton(type: .zoomOut) { _ in
      FrameworkHelper.zoomMap(.out)
      Alohalytics.logEvent(kStatCarplayZoom, with: [kStatIsZoomIn : false,
                                                    kStatIsPanActivated: true])
    }
    mapTemplate.mapButtons = [zoomInButton, zoomOutButton]
    
    let doneButton = buildBarButton(type: .dismissPaning) { _ in
      mapTemplate.dismissPanningInterface(animated: true)
      Alohalytics.logEvent(kStatCarplayPanDeactivated, with: [kStatUsedButtons: CarPlayService.shared.isUserPanMap])
    }
    mapTemplate.leadingNavigationBarButtons = []
    mapTemplate.trailingNavigationBarButtons = [doneButton]
    Alohalytics.logEvent(kStatCarplayPanActivated)
  }
  
  class func configureNavigationUI(mapTemplate: CPMapTemplate) {
    mapTemplate.userInfo = MapInfo(type: CPConstants.TemplateType.navigation)
    let panningButton = buildMapButton(type: .startPanning) { _ in
      mapTemplate.showPanningInterface(animated: true)
    }
    mapTemplate.mapButtons = [panningButton]
    setupMuteAndRedirectButtons(template: mapTemplate)
    let endButton = buildBarButton(type: .endRoute) { _ in
      CarPlayService.shared.cancelCurrentTrip()
    }
    mapTemplate.trailingNavigationBarButtons = [endButton]
  }
  
  // MARK: - Conditional navigation buttons
  class func setupDestinationButton(mapTemplate: CPMapTemplate) {
    let destinationButton = buildBarButton(type: .destination) { _ in
      let listTemplate = ListTemplateBuilder.buildListTemplate(for: .history)
      CarPlayService.shared.pushTemplate(listTemplate, animated: true)
      Statistics.logEvent(kStatCarplayDestinationsOpen, withParameters: [kStatFrom : kStatMap])
    }
    mapTemplate.leadingNavigationBarButtons = [destinationButton]
  }
  
  class func setupRecenterButton(mapTemplate: CPMapTemplate) {
    let recenterButton = buildBarButton(type: .recenter) { _ in
      FrameworkHelper.switchMyPositionMode()
      Alohalytics.logEvent(kStatCarplayRecenter)
    }
    mapTemplate.leadingNavigationBarButtons = [recenterButton]
  }
  
  private class func setupMuteAndRedirectButtons(template: CPMapTemplate) {
    let muteButton = buildBarButton(type: .mute) { _ in
      MWMTextToSpeech.setTTSEnabled(false)
      setupUnmuteAndRedirectButtons(template: template)
    }
    let redirectButton = buildBarButton(type: .redirectRoute) { _ in
      let listTemplate = ListTemplateBuilder.buildListTemplate(for: .history)
      CarPlayService.shared.pushTemplate(listTemplate, animated: true)
      Statistics.logEvent(kStatCarplayDestinationsOpen, withParameters: [kStatFrom : kStatNavigation])
    }
    template.leadingNavigationBarButtons = [muteButton, redirectButton]
  }
  
  private class func setupUnmuteAndRedirectButtons(template: CPMapTemplate) {
    let unmuteButton = buildBarButton(type: .unmute) { _ in
      MWMTextToSpeech.setTTSEnabled(true)
      setupMuteAndRedirectButtons(template: template)
    }
    let redirectButton = buildBarButton(type: .redirectRoute) { _ in
      let listTemplate = ListTemplateBuilder.buildListTemplate(for: .history)
      CarPlayService.shared.pushTemplate(listTemplate, animated: true)
      Statistics.logEvent(kStatCarplayDestinationsOpen, withParameters: [kStatFrom : kStatNavigation])
    }
    template.leadingNavigationBarButtons = [unmuteButton, redirectButton]
  }
  
  // MARK: - CPMapButton builder
  private class func buildMapButton(type: MapButtonType, action: ((CPMapButton) -> Void)?) -> CPMapButton {
    let button = CPMapButton(handler: action)
    switch type {
    case .startPanning:
      button.image = UIImage(named: "btn_carplay_pan_light")
    case .zoomIn:
      button.image = UIImage(named: "btn_zoom_in_light")
    case .zoomOut:
      button.image = UIImage(named: "btn_zoom_out_light")
    }
    return button
  }
  
  // MARK: - CPBarButton builder
  private class func buildBarButton(type: BarButtonType, action: ((CPBarButton) -> Void)?) -> CPBarButton {
    switch type {
    case .dismissPaning:
      let button = CPBarButton(type: .text, handler: action)
      button.title = L("done")
      return button
    case .destination:
      let button = CPBarButton(type: .text, handler: action)
      button.title = L("pick_destination")
      return button
    case .recenter:
      let button = CPBarButton(type: .text, handler: action)
      button.title = L("follow_my_position")
      return button
    case .settings:
      let button = CPBarButton(type: .image, handler: action)
      button.image = UIImage(named: "ic_carplay_settings")
      return button
    case .mute:
      let button = CPBarButton(type: .image, handler: action)
      button.image = UIImage(named: "ic_carplay_unmuted")
      return button
    case .unmute:
      let button = CPBarButton(type: .image, handler: action)
      button.image = UIImage(named: "ic_carplay_muted")
      return button
    case .redirectRoute:
      let button = CPBarButton(type: .image, handler: action)
      button.image = UIImage(named: "ic_carplay_redirect_route")
      return button
    case .endRoute:
      let button = CPBarButton(type: .text, handler: action)
      button.title = L("navigation_stop_button").capitalized
      return button
    }
  }
}
