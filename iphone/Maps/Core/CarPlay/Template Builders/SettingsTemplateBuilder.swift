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

  /// Toggle the displayed value, but load fresh options on tap to preserve changes made on the phone.
  private class func createTollButton(options: RoutingOptions) -> CPGridButton {
    let tollIconName = options.avoidToll ? "ic_carplay_toll_active" : "ic_carplay_toll"
    let avoidToll = !options.avoidToll
    return CPGridButton(titleVariants: [L("avoid_tolls")],
                        image: UIImage(named: tollIconName)!) { _ in
      let options = RoutingOptions()
      options.avoidToll = avoidToll
      options.save()
      CarPlayService.shared.updateRouteAfterChangingSettings()
      CarPlayService.shared.popTemplate(animated: true)
    }
  }

  private class func createUnpavedButton(options: RoutingOptions) -> CPGridButton {
    let unpavedIconName = options.avoidDirty ? "ic_carplay_unpaved_active" : "ic_carplay_unpaved"
    let avoidDirty = !options.avoidDirty
    return CPGridButton(titleVariants: [L("avoid_unpaved")],
                        image: UIImage(named: unpavedIconName)!) { _ in
      let options = RoutingOptions()
      options.avoidDirty = avoidDirty
      options.save()
      CarPlayService.shared.updateRouteAfterChangingSettings()
      CarPlayService.shared.popTemplate(animated: true)
    }
  }

  private class func createFerryButton(options: RoutingOptions) -> CPGridButton {
    let ferryIconName = options.avoidFerry ? "ic_carplay_ferry_active" : "ic_carplay_ferry"
    let avoidFerry = !options.avoidFerry
    return CPGridButton(titleVariants: [L("avoid_ferry")],
                        image: UIImage(named: ferryIconName)!) { _ in
      let options = RoutingOptions()
      options.avoidFerry = avoidFerry
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
