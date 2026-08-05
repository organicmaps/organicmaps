final class RoutingOptionsSettingsPresenter {
  private static let windDirectionLabels = ["route_wind_direction_n", "route_wind_direction_ne",
                                            "route_wind_direction_e", "route_wind_direction_se",
                                            "route_wind_direction_s", "route_wind_direction_sw",
                                            "route_wind_direction_w", "route_wind_direction_nw"]

  private weak var viewController: RoutingOptionsSettingsViewController?

  init(viewController: RoutingOptionsSettingsViewController) {
    self.viewController = viewController
  }

  func present(_ state: RoutingOptionsSettingsState,
               reconfiguredItems: [RoutingOption] = [],
               animatingDifferences: Bool = true) {
    viewController?.display(SettingsViewModel(title: RootSettings.routingOptions.title,
                                              sections: sections(from: state),
                                              reconfiguredItems: reconfiguredItems,
                                              animatingDifferences: animatingDifferences))
  }

  private func sections(from state: RoutingOptionsSettingsState) -> [RoutingOptionsSettingsSectionViewModel] {
    var sections = [SettingsSectionViewModel(section: .options,
                                             items: [.tollRoads, .unpavedRoads, .ferryCrossings, .motorways].map {
                                               item($0, state: state)
                                             })]
    guard let speedSettings = state.speedSettings else { return sections }
    sections.append(SettingsSectionViewModel(section: .routeSpeed,
                                             header: RoutingOptionsSettingsSection.routeSpeed.title,
                                             footer: RoutingOptionsSettingsSection.routeSpeed.footer,
                                             items: [cruisingSpeedItem(state, speedSettings)]))
    guard speedSettings.windSupported else { return sections }
    var windItems = [windEnabledItem(state)]
    if state.windEnabled {
      windItems.append(windSpeedItem(state, speedSettings))
      windItems.append(windDirectionItem(state))
    }
    sections.append(SettingsSectionViewModel(section: .wind,
                                             footer: RoutingOptionsSettingsSection.wind.footer,
                                             items: windItems))
    return sections
  }

  private func item(_ option: RoutingOption,
                    state: RoutingOptionsSettingsState) -> RoutingOptionsSettingsItemViewModel {
    SettingsItemViewModel(item: option,
                          title: option.title,
                          kind: .switcher(isOn: option.isEnabled(in: state.options), isEnabled: true))
  }

  private func cruisingSpeedItem(_ state: RoutingOptionsSettingsState,
                                 _ speedSettings: RouteSpeedSettings) -> RoutingOptionsSettingsItemViewModel {
    SettingsItemViewModel(item: .routeSpeed,
                          kind: .slider(value: Float(state.cruisingSpeedKMpH),
                                        minimumValue: Float(speedSettings.minimumSpeedKMpH),
                                        maximumValue: Float(speedSettings.maximumSpeedKMpH),
                                        valueTitle: cruisingSpeedTitle(state.cruisingSpeedKMpH),
                                        isEnabled: true))
  }

  private func windEnabledItem(_ state: RoutingOptionsSettingsState) -> RoutingOptionsSettingsItemViewModel {
    SettingsItemViewModel(item: .windEnabled,
                          title: RoutingOption.windEnabled.title,
                          kind: .switcher(isOn: state.windEnabled, isEnabled: true))
  }

  private func windSpeedItem(_ state: RoutingOptionsSettingsState,
                             _ speedSettings: RouteSpeedSettings) -> RoutingOptionsSettingsItemViewModel {
    SettingsItemViewModel(item: .windSpeed,
                          title: RoutingOption.windSpeed.title,
                          kind: .slider(value: Float(state.windSpeedMpS),
                                        minimumValue: 1,
                                        maximumValue: Float(speedSettings.maximumWindSpeedMpS),
                                        valueTitle: windSpeedTitle(state.windSpeedMpS),
                                        isEnabled: true))
  }

  private func windDirectionItem(_ state: RoutingOptionsSettingsState) -> RoutingOptionsSettingsItemViewModel {
    let degrees = state.windDirectionDegrees
    let step = RouteSpeedSettings.windDirectionStepDegrees
    let label = L(Self.windDirectionLabels[degrees / step])
    return SettingsItemViewModel(item: .windDirection,
                                 title: RoutingOption.windDirection.title,
                                 kind: .slider(value: Float(degrees),
                                               minimumValue: 0,
                                               maximumValue: Float(360 - step),
                                               valueTitle: "\(label) · \(degrees)°",
                                               isEnabled: true))
  }

  private func cruisingSpeedTitle(_ speedKMpH: Double) -> String {
    let imperial = Settings.measurementUnits() == .imperial
    let value = imperial ? speedKMpH * 0.621371192 : speedKMpH
    let units = imperial ? L("miles_per_hour") : L("kilometers_per_hour")
    let formatter = NumberFormatter()
    formatter.maximumFractionDigits = 1
    return "\(formatter.string(from: NSNumber(value: value)) ?? String(value))\u{00a0}\(units)"
  }

  private func windSpeedTitle(_ speedMpS: Int) -> String {
    guard Settings.measurementUnits() == .imperial else { return "\(speedMpS)\u{00a0}\(L("route_wind_speed_unit_mps"))" }
    let speedMPH = Int((Double(speedMpS) * 2.236936292).rounded())
    return "\(speedMPH)\u{00a0}\(L("miles_per_hour"))"
  }
}
