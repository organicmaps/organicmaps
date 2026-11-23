import CarPlay

final class SettingsTemplateBuilder {
  // MARK: - CPGridTemplate builder

  class func buildGridTemplate() -> CPGridTemplate {
    let actions = SettingsTemplateBuilder.buildGridButtons()
    return CPGridTemplate(title: L("settings"),
                          gridButtons: actions)
  }

  private class func buildGridButtons() -> [CPGridButton] {
    let options = RoutingOptions(routerType: .vehicle)
    return [createUnpavedButton(options: options),
            createTollButton(options: options),
            createFerryButton(options: options),
            createSpeedLimitButton(options: options),
            createSpeedcamButton()]
  }

  // MARK: - CPGridButton builders

  private class func createSpeedLimitButton(options: RoutingOptions) -> CPGridButton {
    let currentSpeedKMpH = options.maxSpeedKMpH
    let titleVariants: [String]
    let sizeConfig = UIImage.SymbolConfiguration(pointSize: 32, weight: .medium)
    let config: UIImage.SymbolConfiguration
    if currentSpeedKMpH > 0 {
      let isImperial = Settings.measurementUnits() == .imperial
      let speedText = isImperial
        ? "\(lround(currentSpeedKMpH / 1.609344)) mph"
        : "\(lround(currentSpeedKMpH)) km/h"
      let fullTitle = "\(NSLocalizedString("speed_limit_title", value: "Speed Limit", comment: "")): \(speedText)"
      titleVariants = [fullTitle, speedText]
      let colorConfig = UIImage.SymbolConfiguration(hierarchicalColor: .systemGreen)
      config = sizeConfig.applying(colorConfig)
    } else {
      titleVariants = [NSLocalizedString("speed_limit_title", value: "Speed Limit", comment: "")]
      config = sizeConfig
    }

    let fallbackName = currentSpeedKMpH > 0 ? "ic_carplay_speedcam_active" : "ic_carplay_speedcam"
    let icon = UIImage(systemName: "speedometer", withConfiguration: config) ?? UIImage(named: fallbackName)!
    return CPGridButton(titleVariants: titleVariants, image: icon) { _ in
      let listTemplate = SettingsTemplateBuilder.buildSpeedLimitTemplate()
      CarPlayService.shared.pushTemplate(listTemplate, animated: true)
    }
  }

  /// Toggle the displayed value, but load fresh options on tap to preserve changes made on the phone.
  private class func createTollButton(options: RoutingOptions) -> CPGridButton {
    let tollIconName = options.avoidToll ? "ic_carplay_toll_active" : "ic_carplay_toll"
    let avoidToll = !options.avoidToll
    return CPGridButton(titleVariants: [L("avoid_tolls")],
                        image: UIImage(named: tollIconName)!) { _ in
      let options = RoutingOptions(routerType: .vehicle)
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
      let options = RoutingOptions(routerType: .vehicle)
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
      let options = RoutingOptions(routerType: .vehicle)
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

  // MARK: - CPListTemplate builders

  class func buildSpeedLimitTemplate(isDriving: Bool = CarPlayService.shared.isDrivingOrMoving) -> CPListTemplate {
    let options = RoutingOptions(routerType: .vehicle)
    let currentSpeedKMpH = options.maxSpeedKMpH
    let isImperial = Settings.measurementUnits() == .imperial

    var items: [CPListItem] = []

    let noLimitTitle = NSLocalizedString("speed_limit_none", value: "No Limit", comment: "")
    let noLimitItem = CPListItem(text: noLimitTitle, detailText: nil)
    if currentSpeedKMpH <= 0 {
      noLimitItem.setAccessoryImage(UIImage(systemName: "checkmark"))
    }
    noLimitItem.handler = { _, completionHandler in
      let options = RoutingOptions(routerType: .vehicle)
      options.maxSpeedKMpH = 0.0
      options.save()
      CarPlayService.shared.updateRouteAfterChangingSettings()
      CarPlayService.shared.popToRootTemplate(animated: true)
      completionHandler()
    }
    items.append(noLimitItem)

    let presets: [Double]
    let formatPreset: (Double) -> String

    if isImperial {
      // 25, 35, 45, 55, 65 mph
      let mphPresets = [25.0, 35.0, 45.0, 55.0, 65.0]
      presets = mphPresets.map { $0 * 1.609344 }
      formatPreset = { speedKMpH in
        let mph = lround(speedKMpH / 1.609344)
        return "\(mph) mph"
      }
    } else {
      // 30, 50, 60, 80, 100 km/h
      presets = [30.0, 50.0, 60.0, 80.0, 100.0]
      formatPreset = { speedKMpH in
        let kmh = lround(speedKMpH)
        return "\(kmh) km/h"
      }
    }

    var matchedPreset = false
    for presetKMpH in presets {
      let isSelected = currentSpeedKMpH > 0 && abs(currentSpeedKMpH - presetKMpH) < 0.5
      if isSelected {
        matchedPreset = true
      }
      let item = CPListItem(text: formatPreset(presetKMpH), detailText: nil)
      if isSelected {
        item.setAccessoryImage(UIImage(systemName: "checkmark"))
      }
      item.handler = { _, completionHandler in
        let options = RoutingOptions(routerType: .vehicle)
        options.maxSpeedKMpH = presetKMpH
        options.save()
        CarPlayService.shared.updateRouteAfterChangingSettings()
        CarPlayService.shared.popToRootTemplate(animated: true)
        completionHandler()
      }
      items.append(item)
    }

    let isCustomSelected = currentSpeedKMpH > 0 && !matchedPreset
    let customTitle = NSLocalizedString("speed_limit_custom", value: "Custom…", comment: "")
    let customText: String
    if isCustomSelected {
      let speedText = isImperial
        ? "\(lround(currentSpeedKMpH / 1.609344)) mph"
        : "\(lround(currentSpeedKMpH)) km/h"
      customText = "\(customTitle): \(speedText)"
    } else {
      customText = customTitle
    }

    let customDetail = isDriving ? L("keyboard_availability_alert") : nil
    let customItem = CPListItem(text: customText, detailText: customDetail)
    if isCustomSelected {
      customItem.setAccessoryImage(UIImage(systemName: "checkmark"))
    }

    if isDriving {
      customItem.isEnabled = false
      customItem.handler = { _, completionHandler in
        CarPlayService.shared.showKeyboardAlert()
        completionHandler()
      }
    } else {
      customItem.isEnabled = true
      customItem.handler = { _, completionHandler in
        let customTemplate = SettingsTemplateBuilder.buildCustomSpeedLimitTemplate()
        CarPlayService.shared.pushTemplate(customTemplate, animated: true)
        completionHandler()
      }
    }
    items.append(customItem)

    let section = CPListSection(items: items)
    let title = NSLocalizedString("speed_limit_header", value: "Speed Limit", comment: "")
    return CPListTemplate(title: title, sections: [section])
  }

  class func buildCustomSpeedLimitTemplate() -> CPListTemplate {
    let options = RoutingOptions(routerType: .vehicle)
    let currentSpeedKMpH = options.maxSpeedKMpH
    let isImperial = Settings.measurementUnits() == .imperial

    var items: [CPListItem] = []

    let values: [Double]
    let formatValue: (Double) -> String

    if isImperial {
      // 15 to 85 mph in 5 mph increments
      let mphValues = stride(from: 15.0, through: 85.0, by: 5.0).map { $0 }
      values = mphValues.map { $0 * 1.609344 }
      formatValue = { speedKMpH in
        let mph = lround(speedKMpH / 1.609344)
        return "\(mph) mph"
      }
    } else {
      // 20 to 140 km/h in standard legal increments
      let kmhValues = [20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0,
                       70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0, 105.0, 110.0, 115.0,
                       120.0, 130.0, 140.0]
      values = kmhValues
      formatValue = { speedKMpH in
        let kmh = lround(speedKMpH)
        return "\(kmh) km/h"
      }
    }

    if currentSpeedKMpH > 0 && !values.contains(where: { abs($0 - currentSpeedKMpH) < 0.5 }) {
      let currentCustomText = isImperial
        ? "\(lround(currentSpeedKMpH / 1.609344)) mph"
        : "\(lround(currentSpeedKMpH)) km/h"
      let currentItem = CPListItem(text: currentCustomText,
                                   detailText: NSLocalizedString("speed_limit_custom", value: "Custom…", comment: ""))
      currentItem.setAccessoryImage(UIImage(systemName: "checkmark"))
      currentItem.handler = { _, completionHandler in
        let options = RoutingOptions(routerType: .vehicle)
        options.maxSpeedKMpH = currentSpeedKMpH
        options.save()
        CarPlayService.shared.updateRouteAfterChangingSettings()
        CarPlayService.shared.popToRootTemplate(animated: true)
        completionHandler()
      }
      items.append(currentItem)
    }

    for valKMpH in values {
      let isSelected = currentSpeedKMpH > 0 && abs(currentSpeedKMpH - valKMpH) < 0.5
      let item = CPListItem(text: formatValue(valKMpH), detailText: nil)
      if isSelected {
        item.setAccessoryImage(UIImage(systemName: "checkmark"))
      }
      item.handler = { _, completionHandler in
        if CarPlayService.shared.isDrivingOrMoving {
          CarPlayService.shared.showKeyboardAlert()
          completionHandler()
          return
        }
        let options = RoutingOptions(routerType: .vehicle)
        options.maxSpeedKMpH = valKMpH
        options.save()
        CarPlayService.shared.updateRouteAfterChangingSettings()
        CarPlayService.shared.popToRootTemplate(animated: true)
        completionHandler()
      }
      items.append(item)
    }

    let section = CPListSection(items: items)
    let title = NSLocalizedString("custom_speed_limit", value: "Custom Speed Limit", comment: "")
    return CPListTemplate(title: title, sections: [section])
  }
}
