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
  case steps
  case difficultTrails
  case maxSpeed
}

extension RoutingOption {
  static let avoidanceOptions: [RoutingOption] = [.tollRoads, .unpavedRoads, .ferryCrossings, .motorways, .steps, .difficultTrails]

  func isSupported(in options: RoutingOptions) -> Bool {
    switch self {
    case .routeOptimization: return true
    case .tollRoads: return options.supportedOptions.contains(.toll)
    case .unpavedRoads: return options.supportedOptions.contains(.dirty)
    case .ferryCrossings: return options.supportedOptions.contains(.ferry)
    case .motorways: return options.supportedOptions.contains(.motorway)
    case .steps: return options.supportedOptions.contains(.steps)
    case .difficultTrails: return options.supportedOptions.contains(.difficultTrail)
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
    case .steps: return L("avoid_steps")
    case .difficultTrails: return L("avoid_difficult_trails")
    case .maxSpeed: return NSLocalizedString("maximum_speed_limit", value: "Maximum Speed", comment: "")
    }
  }

  func isEnabled(in options: RoutingOptions) -> Bool {
    switch self {
    case .maxSpeed: return options.maxSpeedKMpH > 0
    default: return options[keyPath: routingOptionsKeyPath]
    }
  }

  func setEnabled(_ enabled: Bool, in options: RoutingOptions) {
    switch self {
    case .maxSpeed: break
    default: options[keyPath: routingOptionsKeyPath] = enabled
    }
  }

  private var routingOptionsKeyPath: ReferenceWritableKeyPath<RoutingOptions, Bool> {
    switch self {
    case .routeOptimization: return \.routeOptimizationEnabled
    case .tollRoads: return \.avoidToll
    case .unpavedRoads: return \.avoidDirty
    case .ferryCrossings: return \.avoidFerry
    case .motorways: return \.avoidMotorway
    case .steps: return \.avoidSteps
    case .difficultTrails: return \.avoidDifficultTrails
    case .maxSpeed: fatalError("maxSpeed is not a boolean option")
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
