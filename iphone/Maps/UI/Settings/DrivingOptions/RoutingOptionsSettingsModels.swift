enum RoutingOptionsSettingsSection: String {
  case options
  case optimization
}

enum RoutingOption: String {
  case routeOptimization
  case tollRoads
  case unpavedRoads
  case ferryCrossings
  case motorways
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
    }
  }

  var title: String {
    switch self {
    case .routeOptimization: return L("route_optimization")
    case .tollRoads: return L("avoid_tolls")
    case .unpavedRoads: return L("avoid_unpaved")
    case .ferryCrossings: return L("avoid_ferry")
    case .motorways: return L("avoid_motorways")
    }
  }

  func isEnabled(in options: RoutingOptions) -> Bool {
    options[keyPath: routingOptionsKeyPath]
  }

  func setEnabled(_ enabled: Bool, in options: RoutingOptions) {
    options[keyPath: routingOptionsKeyPath] = enabled
  }

  private var routingOptionsKeyPath: ReferenceWritableKeyPath<RoutingOptions, Bool> {
    switch self {
    case .routeOptimization: return \.routeOptimizationEnabled
    case .tollRoads: return \.avoidToll
    case .unpavedRoads: return \.avoidDirty
    case .ferryCrossings: return \.avoidFerry
    case .motorways: return \.avoidMotorway
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
