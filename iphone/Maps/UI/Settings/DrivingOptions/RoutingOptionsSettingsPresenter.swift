final class RoutingOptionsSettingsPresenter {
  private weak var viewController: RoutingOptionsSettingsViewController?

  init(viewController: RoutingOptionsSettingsViewController) {
    self.viewController = viewController
  }

  func present(_ state: RoutingOptionsSettingsState,
               animatingDifferences: Bool = true) {
    viewController?.display(SettingsViewModel(title: state.options.routerType.routingOptionsTitle,
                                              sections: sections(from: state),
                                              animatingDifferences: animatingDifferences))
  }

  func sections(from state: RoutingOptionsSettingsState) -> [RoutingOptionsSettingsSectionViewModel] {
    let avoidances = RoutingOption.avoidanceOptions.filter { $0.isSupported(in: state.options) }
    var sections: [RoutingOptionsSettingsSectionViewModel] = []
    if !avoidances.isEmpty {
      sections.append(SettingsSectionViewModel(section: .options,
                                               items: avoidances.map { item($0, state: state) }))
    }

    if state.options.routerType == .vehicle {
      let speedText: String
      let currentSpeed = state.options.maxSpeedKMpH
      if currentSpeed > 0 {
        let isImperial = Settings.measurementUnits() == .imperial
        if isImperial {
          let mph = lround(currentSpeed / 1.609344)
          speedText = "\(mph) mph"
        } else {
          let kmh = lround(currentSpeed)
          speedText = "\(kmh) km/h"
        }
      } else {
        speedText = NSLocalizedString("speed_limit_none", value: "No Limit", comment: "")
      }

      sections.append(SettingsSectionViewModel(section: .speedLimit,
                                               header: NSLocalizedString("speed_limit_header", value: "Speed Limit", comment: ""),
                                               footer: NSLocalizedString("speed_limit_description",
                                                                         value: "Prefer roads at or below this speed limit (best effort).",
                                                                         comment: ""),
                                               items: [SettingsItemViewModel(item: .maxSpeed,
                                                                             title: RoutingOption.maxSpeed.title,
                                                                             detail: speedText,
                                                                             kind: .link)]))
    }

    sections.append(SettingsSectionViewModel(section: .optimization,
                                             header: L("routing_options_all_modes"),
                                             footer: L("route_optimization_description"),
                                             items: [item(.routeOptimization, state: state)]))
    return sections
  }

  func presentSpeedLimitPicker(currentKMpH: Double, onSelect: @escaping (Double) -> Void) {
    let isImperial = Settings.measurementUnits() == .imperial
    let title = NSLocalizedString("maximum_speed_limit", value: "Maximum Speed", comment: "")
    let message = NSLocalizedString("speed_limit_picker_message",
                                    value: "Only route on roads with at most this speed limit (best effort)",
                                    comment: "")
    let alert = UIAlertController(title: title, message: message, preferredStyle: .actionSheet)

    // Option: No Limit
    let noLimitTitle = (currentKMpH <= 0 ? "✓ " : "") + NSLocalizedString("speed_limit_none", value: "No Limit", comment: "")
    alert.addAction(UIAlertAction(title: noLimitTitle, style: .default) { _ in
      onSelect(0.0)
    })

    // Presets
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

    for presetKMpH in presets {
      let isSelected = abs(currentKMpH - presetKMpH) < 0.5
      let prefix = isSelected ? "✓ " : ""
      let actionTitle = prefix + formatPreset(presetKMpH)
      alert.addAction(UIAlertAction(title: actionTitle, style: .default) { _ in
        onSelect(presetKMpH)
      })
    }

    // Custom... option
    let customTitle = NSLocalizedString("speed_limit_custom", value: "Custom…", comment: "")
    alert.addAction(UIAlertAction(title: customTitle, style: .default) { [weak self] _ in
      self?.presentCustomSpeedLimitInput(isImperial: isImperial, onSelect: onSelect)
    })

    alert.addAction(UIAlertAction(title: L("cancel"), style: .cancel))
    viewController?.display(alert)
  }

  private func presentCustomSpeedLimitInput(isImperial: Bool, onSelect: @escaping (Double) -> Void) {
    let unitStr = isImperial ? "mph" : "km/h"
    let title = NSLocalizedString("custom_speed_limit", value: "Custom Speed Limit", comment: "")
    let message = "\(NSLocalizedString("enter_max_speed", value: "Enter maximum speed in", comment: "")) \(unitStr)"
    let alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
    alert.addTextField { textField in
      textField.keyboardType = .numberPad
      textField.placeholder = isImperial ? "35" : "50"
    }
    alert.addAction(UIAlertAction(title: L("cancel"), style: .cancel))
    alert.addAction(UIAlertAction(title: "OK", style: .default) { [weak alert] _ in
      guard let text = alert?.textFields?.first?.text,
            let val = Double(text.trimmingCharacters(in: .whitespacesAndNewlines)),
            val > 0 else { return }
      let kmph = isImperial ? val * 1.609344 : val
      onSelect(kmph)
    })
    viewController?.display(alert)
  }

  private func item(_ option: RoutingOption,
                    state: RoutingOptionsSettingsState) -> RoutingOptionsSettingsItemViewModel {
    SettingsItemViewModel(item: option,
                          title: option.title,
                          kind: .switcher(isOn: option.isEnabled(in: state.options),
                                          isEnabled: option != .routeOptimization || state.canChangeOptimization))
  }
}
