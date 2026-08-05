enum RoutingOptionsSettingsSection: String {
  case options
  case routeSpeed
  case wind
}

extension RoutingOptionsSettingsSection {
  var title: String? {
    switch self {
    case .options: return nil
    case .routeSpeed: return L("route_speed_title")
    case .wind: return nil
    }
  }

  var footer: String? {
    switch self {
    case .options: return nil
    case .routeSpeed: return L("route_speed_description")
    case .wind: return L("route_wind_description")
    }
  }
}

enum RoutingOption: String, CaseIterable {
  case tollRoads
  case unpavedRoads
  case ferryCrossings
  case motorways
  case routeSpeed
  case windEnabled
  case windSpeed
  case windDirection
}

extension RoutingOption {
  var title: String {
    switch self {
    case .tollRoads: return L("avoid_tolls")
    case .unpavedRoads: return L("avoid_unpaved")
    case .ferryCrossings: return L("avoid_ferry")
    case .motorways: return L("avoid_motorways")
    case .routeSpeed: return L("route_speed_title")
    case .windEnabled: return L("route_wind_enabled")
    case .windSpeed: return L("route_wind_speed")
    case .windDirection: return L("route_wind_direction")
    }
  }

  func isEnabled(in options: RoutingOptions) -> Bool {
    guard let routingOptionsKeyPath else { return false }
    return options[keyPath: routingOptionsKeyPath]
  }

  func setEnabled(_ enabled: Bool, in options: RoutingOptions) {
    guard let routingOptionsKeyPath else { return }
    options[keyPath: routingOptionsKeyPath] = enabled
  }

  private var routingOptionsKeyPath: ReferenceWritableKeyPath<RoutingOptions, Bool>? {
    switch self {
    case .tollRoads: return \.avoidToll
    case .unpavedRoads: return \.avoidDirty
    case .ferryCrossings: return \.avoidFerry
    case .motorways: return \.avoidMotorway
    case .routeSpeed, .windEnabled, .windSpeed, .windDirection: return nil
    }
  }
}

struct RoutingOptionsSettingsState {
  let options: RoutingOptions
  let speedSettings: RouteSpeedSettings?
  var cruisingSpeedKMpH: Double
  var windEnabled: Bool
  var windSpeedMpS: Int
  var windDirectionDegrees: Int

  init(options: RoutingOptions, speedSettings: RouteSpeedSettings?) {
    self.options = options
    self.speedSettings = speedSettings
    let savedWindSpeedMpS = speedSettings?.windSpeedMpS ?? 0
    cruisingSpeedKMpH = speedSettings?.cruisingSpeedKMpH ?? 0
    windEnabled = savedWindSpeedMpS > 0
    windSpeedMpS = windEnabled ? savedWindSpeedMpS : RouteSpeedSettings.defaultWindSpeedMpS
    windDirectionDegrees = speedSettings?.windDirectionDegrees ?? 0
  }

  var windSpeedToApplyMpS: Int { windEnabled ? windSpeedMpS : 0 }

  var isSpeedChanged: Bool {
    guard let speedSettings else { return false }
    return cruisingSpeedKMpH != speedSettings.cruisingSpeedKMpH || windSpeedToApplyMpS != speedSettings.windSpeedMpS ||
           (windEnabled && windDirectionDegrees != speedSettings.windDirectionDegrees)
  }
}

typealias RoutingOptionsSettingsViewController = SettingsViewController<RoutingOptionsSettingsSection, RoutingOption>
typealias RoutingOptionsSettingsSectionViewModel = SettingsSectionViewModel<RoutingOptionsSettingsSection, RoutingOption>
typealias RoutingOptionsSettingsItemViewModel = SettingsItemViewModel<RoutingOption>
