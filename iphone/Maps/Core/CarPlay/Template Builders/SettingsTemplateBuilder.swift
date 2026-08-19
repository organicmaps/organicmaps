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
    let tollIconName = options.avoidToll ? "ic_carplay_toll_active" : "ic_carplay_toll"
    return CPGridButton(titleVariants: [L("avoid_tolls")],
                        image: UIImage(named: tollIconName)!) { _ in
      options.avoidToll = !options.avoidToll
      options.save()
      CarPlayService.shared.updateRouteAfterChangingSettings()
      CarPlayService.shared.popTemplate(animated: true)
    }
  }

  private class func createUnpavedButton(options: RoutingOptions) -> CPGridButton {
    let unpavedIconName = options.avoidDirty ? "ic_carplay_unpaved_active" : "ic_carplay_unpaved"
    return CPGridButton(titleVariants: [L("avoid_unpaved")],
                        image: UIImage(named: unpavedIconName)!) { _ in
      options.avoidDirty = !options.avoidDirty
      options.save()
      CarPlayService.shared.updateRouteAfterChangingSettings()
      CarPlayService.shared.popTemplate(animated: true)
    }
  }

  private class func createFerryButton(options: RoutingOptions) -> CPGridButton {
    let ferryIconName = options.avoidFerry ? "ic_carplay_ferry_active" : "ic_carplay_ferry"
    return CPGridButton(titleVariants: [L("avoid_ferry")],
                        image: UIImage(named: ferryIconName)!) { _ in
      options.avoidFerry = !options.avoidFerry
      options.save()
      CarPlayService.shared.updateRouteAfterChangingSettings()
      CarPlayService.shared.popTemplate(animated: true)
    }
  }

  private class func createSpeedcamButton() -> CPGridButton {
    let isSpeedCamActivated = CarPlayService.shared.isSpeedCamActivated
    let speedcamIconName = isSpeedCamActivated ? "ic_carplay_speedcam_active" : "ic_carplay_speedcam"
    return CPGridButton(titleVariants: [L("speedcams_alert_title_carplay_1"),
                                        L("speedcams_alert_title_carplay_2")],
                        image: UIImage(named: speedcamIconName)!) { _ in
      CarPlayService.shared.isSpeedCamActivated = !isSpeedCamActivated
      CarPlayService.shared.popTemplate(animated: true)
    }
  }
}
