import CarPlay

final class SettingsTemplateBuilder {
  // MARK: - CPGridTemplate builder
  class func buildGridTemplate() -> CPGridTemplate {
    let actions = SettingsTemplateBuilder.buildGridButtons()
    let gridTemplate = CPGridTemplate(title: L("settings"),
                                      gridButtons: actions)
    
    return gridTemplate
  }
  
  private class func buildGridButtons() -> [CPGridButton] {
    let options = RoutingOptions()
    return [createUnpavedButton(options: options),
            createTollButton(options: options),
            createFerryButton(options: options),
            createTrafficButton(),
            createSpeedcamButton()]
  }
  
  // MARK: - CPGridButton builders
  private class func createTollButton(options: RoutingOptions) -> CPGridButton {
    var tollIconName = "ic_carplay_toll"
    if options.avoidToll { tollIconName += "_active" }
    let tollButton = CPGridButton(titleVariants: [L("avoid_tolls_carplay_1"),
                                                  L("avoid_tolls_carplay_2")],
                                  image: UIImage(named: tollIconName)!) { _ in
                                    options.avoidToll = !options.avoidToll
                                    options.save()
                                    CarPlayService.shared.updateRouteAfterChangingSettings()
                                    CarPlayService.shared.popTemplate(animated: true)
    }
    return tollButton
  }
  
  private class func createUnpavedButton(options: RoutingOptions) -> CPGridButton {
    var unpavedIconName = "ic_carplay_unpaved"
    if options.avoidDirty { unpavedIconName += "_active" }
    let unpavedButton = CPGridButton(titleVariants: [L("avoid_unpaved_carplay_1"),
                                                     L("avoid_unpaved_carplay_2")],
                                     image: UIImage(named: unpavedIconName)!) { _ in
                                      options.avoidDirty = !options.avoidDirty
                                      options.save()
                                      CarPlayService.shared.updateRouteAfterChangingSettings()
                                      CarPlayService.shared.popTemplate(animated: true)
    }
    return unpavedButton
  }
  
  private class func createFerryButton(options: RoutingOptions) -> CPGridButton {
    var ferryIconName = "ic_carplay_ferry"
    if options.avoidFerry { ferryIconName += "_active" }
    let ferryButton = CPGridButton(titleVariants: [L("avoid_ferry_carplay_1"),
                                                   L("avoid_ferry_carplay_2")],
                                   image: UIImage(named: ferryIconName)!) { _ in
                                    options.avoidFerry = !options.avoidFerry
                                    options.save()
                                    CarPlayService.shared.updateRouteAfterChangingSettings()
                                    CarPlayService.shared.popTemplate(animated: true)
    }
    return ferryButton
  }
  
  private class func createTrafficButton() -> CPGridButton {
    var trafficIconName = "ic_carplay_trafficlight"
    let isTrafficEnabled = MapOverlayManager.trafficEnabled()
    if isTrafficEnabled { trafficIconName += "_active" }
    let trafficButton = CPGridButton(titleVariants: [L("button_layer_traffic")],
                                     image: UIImage(named: trafficIconName)!) { _ in
                                      MapOverlayManager.setTrafficEnabled(!isTrafficEnabled)
                                      CarPlayService.shared.popTemplate(animated: true)
    }
    return trafficButton
  }
  
  private class func createSpeedcamButton() -> CPGridButton {
    var speedcamIconName = "ic_carplay_speedcam"
    let isSpeedCamActivated = CarPlayService.shared.isSpeedCamActivated
    if isSpeedCamActivated { speedcamIconName += "_active" }
    let speedButton = CPGridButton(titleVariants: [L("speedcams_alert_title_carplay_1"),
                                                   L("speedcams_alert_title_carplay_2")],
                                   image: UIImage(named: speedcamIconName)!) { _ in
                                    CarPlayService.shared.isSpeedCamActivated = !isSpeedCamActivated
                                    CarPlayService.shared.popTemplate(animated: true)
    }
    return speedButton
  }
}
