import CarPlay

final class SettingsTemplateBuilder {
  // MARK: - CPGridTemplate builder

  class func buildGridTemplate() -> CPGridTemplate {
    let actions = SettingsTemplateBuilder.buildGridButtons()
    return CPGridTemplate(title: L("settings"),
                          gridButtons: actions)
  }

  private class func buildGridButtons() -> [CPGridButton] {
    let options = RoutingOptions()
    return [createUnpavedButton(options: options),
            createTollButton(options: options),
            createFerryButton(options: options),
            createSpeedcamButton()]
  }

  // MARK: - CPGridButton builders

  private class func createTollButton(options: RoutingOptions) -> CPGridButton {
    var tollIconName = "ic_carplay_toll"
    if options.avoidToll { tollIconName += "_active" }
    return CPGridButton(titleVariants: [L("avoid_tolls")],
                        image: UIImage(named: tollIconName)!) { _ in
      options.avoidToll = !options.avoidToll
      options.save()
      CarPlayService.shared.updateRouteAfterChangingSettings()
      CarPlayService.shared.popTemplate(animated: true)
    }
  }

  private class func createUnpavedButton(options: RoutingOptions) -> CPGridButton {
    var unpavedIconName = "ic_carplay_unpaved"
    if options.avoidDirty { unpavedIconName += "_active" }
    return CPGridButton(titleVariants: [L("avoid_unpaved")],
                        image: UIImage(named: unpavedIconName)!) { _ in
      options.avoidDirty = !options.avoidDirty
      options.save()
      CarPlayService.shared.updateRouteAfterChangingSettings()
      CarPlayService.shared.popTemplate(animated: true)
    }
  }

  private class func createFerryButton(options: RoutingOptions) -> CPGridButton {
    var ferryIconName = "ic_carplay_ferry"
    if options.avoidFerry { ferryIconName += "_active" }
    return CPGridButton(titleVariants: [L("avoid_ferry")],
                        image: UIImage(named: ferryIconName)!) { _ in
      options.avoidFerry = !options.avoidFerry
      options.save()
      CarPlayService.shared.updateRouteAfterChangingSettings()
      CarPlayService.shared.popTemplate(animated: true)
    }
  }

  private class func createSpeedcamButton() -> CPGridButton {
    var speedcamIconName = "ic_carplay_speedcam"
    let isSpeedCamActivated = CarPlayService.shared.isSpeedCamActivated
    if isSpeedCamActivated { speedcamIconName += "_active" }
    return CPGridButton(titleVariants: [L("speedcams_alert_title_carplay_1"),
                                        L("speedcams_alert_title_carplay_2")],
                        image: UIImage(named: speedcamIconName)!) { _ in
      CarPlayService.shared.isSpeedCamActivated = !isSpeedCamActivated
      CarPlayService.shared.popTemplate(animated: true)
    }
  }
}
