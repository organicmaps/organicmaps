enum RoutingOptionsSettingsSection: String {
  case options
  case speedLimit
  case optimization
}

enum RoutingOption: String {
  case routeOptimization
  case tollRoads
  case unpavedRoads
  case ferryCrossings
  case motorways
  case maxSpeed
}

extension RoutingOption {
  static let avoidanceOptions: [RoutingOption] = [.tollRoads, .unpavedRoads, .ferryCrossings, .motorways]

  func isSupported(in options: RoutingOptions) -> Bool {
    switch self {
    case .routeOptimization: return true
    case .tollRoads: return options.supportedOptions.contains(.toll)
    case .unpavedRoads: return options.supportedOptions.contains(.dirty)
    case .ferryCrossings: return options.supportedOptions.contains(.ferry)
    case .motorways: return options.supportedOptions.contains(.motorway)
    case .maxSpeed: return options.routerType == .vehicle
    }
  }

  var title: String {
    switch self {
    case .routeOptimization: return L("route_optimization")
    case .tollRoads: return L("avoid_tolls")
    case .unpavedRoads: return L("avoid_unpaved")
    case .ferryCrossings: return L("avoid_ferry")
    case .motorways: return L("avoid_motorways")
    case .maxSpeed: return NSLocalizedString("maximum_speed_limit", value: "Maximum Speed", comment: "")
    }
  }

  func isEnabled(in options: RoutingOptions) -> Bool {
    switch self {
    case .routeOptimization: return options.routeOptimizationEnabled
    case .tollRoads: return options.avoidToll
    case .unpavedRoads: return options.avoidDirty
    case .ferryCrossings: return options.avoidFerry
    case .motorways: return options.avoidMotorway
    case .maxSpeed: return options.maxSpeedKMpH > 0
    }
  }

  func setEnabled(_ enabled: Bool, in options: RoutingOptions) {
    switch self {
    case .routeOptimization: options.routeOptimizationEnabled = enabled
    case .tollRoads: options.avoidToll = enabled
    case .unpavedRoads: options.avoidDirty = enabled
    case .ferryCrossings: options.avoidFerry = enabled
    case .motorways: options.avoidMotorway = enabled
    case .maxSpeed: break
    }
  }
}

struct RoutingOptionsSettingsState {
  let options: RoutingOptions
  let canChangeOptimization: Bool
}

typealias RoutingOptionsSettingsViewController = SettingsViewController<RoutingOptionsSettingsSection, RoutingOption>
typealias RoutingOptionsSettingsSectionViewModel = SettingsSectionViewModel<RoutingOptionsSettingsSection, RoutingOption>
typealias RoutingOptionsSettingsItemViewModel = SettingsItemViewModel<RoutingOption>

extension MWMRouterType {
  var hasRoutingSettings: Bool {
    // Transit exposes global optimization without introducing road avoidances.
    self != .ruler
  }

  var routingOptionsTitle: String {
    switch self {
    case .vehicle: return L("routing_options_driving")
    case .bicycle: return L("routing_options_cycling")
    case .pedestrian: return L("routing_options_walking")
    default: return L("driving_options_title")
    }
  }
}
