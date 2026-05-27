import CarPlay

final class MapTemplateBuilder {
  enum MapButtonType {
    case startPanning
    case zoomIn
    case zoomOut
    case recenter
  }

  enum BarButtonType {
    case dismissPaning
    case destination
    case settings
    case mute
    case unmute
    case redirectRoute
    case endRoute
  }

  private enum Constants {
    static let carPlayGuidanceBackgroundColor = UIColor(46, 100, 51, 1.0)
  }

  // MARK: - CPMapTemplate builders

  class func buildBaseTemplate(positionMode: MWMMyPositionMode, isOnRoute: Bool) -> CPMapTemplate {
    let mapTemplate = CPMapTemplate()
    mapTemplate.hidesButtonsWithNavigationBar = true
    configureBaseUI(mapTemplate, positionMode: positionMode, isOnRoute: isOnRoute)
    return mapTemplate
  }

  class func buildNavigationTemplate(positionMode: MWMMyPositionMode) -> CPMapTemplate {
    let mapTemplate = CPMapTemplate()
    mapTemplate.hidesButtonsWithNavigationBar = true
    configureNavigationUI(mapTemplate, positionMode: positionMode)
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
    }
    mapTemplate.trailingNavigationBarButtons = [settingsButton]
    return mapTemplate
  }

  // MARK: - MapTemplate UI configs

  class func configureBaseUI(_ mapTemplate: CPMapTemplate, positionMode: MWMMyPositionMode, isOnRoute: Bool) {
    mapTemplate.userInfo = MapInfo(type: CPConstants.TemplateType.main)
    setupMapButtons(mapTemplate, positionMode: positionMode)
    setupLeadingNavigationBarButtons(mapTemplate, positionMode: positionMode, isOnRoute: isOnRoute)
    let settingsButton = buildBarButton(type: .settings) { _ in
      let gridTemplate = SettingsTemplateBuilder.buildGridTemplate()
      CarPlayService.shared.pushTemplate(gridTemplate, animated: true)
    }
    mapTemplate.trailingNavigationBarButtons = [settingsButton]
  }

  class func configurePanUI(_ mapTemplate: CPMapTemplate) {
    let zoomInButton = buildMapButton(type: .zoomIn) { _ in
      FrameworkHelper.zoomMap(.in)
    }
    let zoomOutButton = buildMapButton(type: .zoomOut) { _ in
      FrameworkHelper.zoomMap(.out)
    }
    mapTemplate.mapButtons = [zoomInButton, zoomOutButton]

    let doneButton = buildBarButton(type: .dismissPaning) { _ in
      mapTemplate.dismissPanningInterface(animated: true)
    }
    mapTemplate.leadingNavigationBarButtons = []
    mapTemplate.trailingNavigationBarButtons = [doneButton]
  }

  class func configureNavigationUI(_ mapTemplate: CPMapTemplate, positionMode: MWMMyPositionMode) {
    mapTemplate.userInfo = MapInfo(type: CPConstants.TemplateType.navigation)
    setupMapButtons(mapTemplate, positionMode: positionMode)
    setupLeadingNavigationBarButtons(mapTemplate, positionMode: positionMode, isOnRoute: true)
    let endButton = buildBarButton(type: .endRoute) { _ in
      CarPlayService.shared.cancelCurrentTrip()
    }
    mapTemplate.trailingNavigationBarButtons = [endButton]
    mapTemplate.guidanceBackgroundColor = Constants.carPlayGuidanceBackgroundColor
  }

  // MARK: - Conditional navigation buttons

  class func setupMapButtons(_ mapTemplate: CPMapTemplate, positionMode: MWMMyPositionMode) {
    let panningButton = buildMapButton(type: .startPanning) { _ in
      mapTemplate.showPanningInterface(animated: true)
    }
    let zoomInButton = buildMapButton(type: .zoomIn) { _ in
      FrameworkHelper.zoomMap(.in)
    }
    let zoomOutButton = buildMapButton(type: .zoomOut) { _ in
      FrameworkHelper.zoomMap(.out)
    }
    let recenterButton = buildMapButton(type: .recenter) { _ in
      mapTemplate.hidesButtonsWithNavigationBar = true
      FrameworkHelper.switchMyPositionMode()
    }

    switch positionMode {
    case .follow, .followAndRotate:
      if !mapTemplate.isPanningInterfaceVisible {
        mapTemplate.mapButtons = [panningButton, zoomInButton, zoomOutButton]
      }
    case .notFollow:
      if !mapTemplate.isPanningInterfaceVisible {
        mapTemplate.mapButtons = [recenterButton, zoomInButton, zoomOutButton]
      }
    case .pendingPosition, .notFollowNoPosition:
      mapTemplate.mapButtons = [panningButton, zoomInButton, zoomOutButton]
    }
  }

  class func setupLeadingNavigationBarButtons(_ mapTemplate: CPMapTemplate, positionMode: MWMMyPositionMode, isOnRoute: Bool) {
    let isTTSEnabled = MWMTextToSpeech.isTTSEnabled()
    let muteButton = buildBarButton(type: isTTSEnabled ? .mute : .unmute) { _ in
      MWMTextToSpeech.setTTSEnabled(!isTTSEnabled)
      setupLeadingNavigationBarButtons(mapTemplate, positionMode: positionMode, isOnRoute: isOnRoute)
    }
    let redirectButton = buildBarButton(type: .redirectRoute) { _ in
      let listTemplate = ListTemplateBuilder.buildListTemplate(for: .history)
      CarPlayService.shared.pushTemplate(listTemplate, animated: true)
    }
    let destinationButton = buildBarButton(type: .destination) { _ in
      let listTemplate = ListTemplateBuilder.buildListTemplate(for: .history)
      CarPlayService.shared.pushTemplate(listTemplate, animated: true)
    }
    if isOnRoute {
      mapTemplate.leadingNavigationBarButtons = [muteButton, redirectButton]
      return
    }
    switch positionMode {
    case .follow, .followAndRotate, .notFollow:
      if !mapTemplate.isPanningInterfaceVisible {
        mapTemplate.leadingNavigationBarButtons = [destinationButton]
      }
    case .pendingPosition, .notFollowNoPosition:
      mapTemplate.leadingNavigationBarButtons = []
    }
  }

  // MARK: - CPMapButton builder

  private class func buildMapButton(type: MapButtonType, action: ((CPMapButton) -> Void)?) -> CPMapButton {
    let button = CPMapButton(handler: action)
    switch type {
    case .startPanning:
      button.image = UIImage.btnCarplayPanLight
    case .zoomIn:
      button.image = UIImage.btnZoomIn
    case .zoomOut:
      button.image = UIImage.btnZoomOut
    case .recenter:
      button.image = UIImage.btnGetPosition
    }
    return button
  }

  // MARK: - CPBarButton builder

  private class func buildBarButton(type: BarButtonType, action: ((CPBarButton) -> Void)?) -> CPBarButton {
    switch type {
    case .dismissPaning:
      CPBarButton(title: L("done"), handler: action)
    case .destination:
      CPBarButton(title: L("pick_destination"), handler: action)
    case .settings:
      CPBarButton(image: UIImage.icCarplaySettings, handler: action)
    case .mute:
      CPBarButton(image: UIImage.icCarplayUnmuted, handler: action)
    case .unmute:
      CPBarButton(image: UIImage.icCarplayMuted, handler: action)
    case .redirectRoute:
      CPBarButton(image: UIImage.icCarplayRedirectRoute, handler: action)
    case .endRoute:
      CPBarButton(title: L("navigation_stop_button").capitalized, handler: action)
    }
  }
}
