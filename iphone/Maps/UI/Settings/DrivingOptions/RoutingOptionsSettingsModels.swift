enum RoutingOptionsSettingsSection: String {
  case options
  case bicycleSurface
  case optimization
}

enum RoutingOption: Hashable {
  case routeOptimization
  case tollRoads
  case unpavedRoads
  case ferryCrossings
  case motorways
  case steps
  case difficultTrails
  case bicycleSurface(BicycleSurfacePreference)
}

extension RoutingOption {
  static let avoidanceOptions: [RoutingOption] = [.tollRoads, .unpavedRoads, .ferryCrossings, .motorways, .steps, .difficultTrails]

  func isSupported(in options: RoutingOptions) -> Bool {
    switch self {
    case .routeOptimization: return true
    case .tollRoads: return options.supportedOptions.contains(.toll)
    case .unpavedRoads: return options.routerType != .bicycle && options.supportedOptions.contains(.dirty)
    case .ferryCrossings: return options.supportedOptions.contains(.ferry)
    case .motorways: return options.supportedOptions.contains(.motorway)
    case .steps: return options.supportedOptions.contains(.steps)
    case .difficultTrails: return options.supportedOptions.contains(.difficultTrail)
    case .bicycleSurface: return options.routerType == .bicycle
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
    case .bicycleSurface(let preference):
      switch preference {
      case .any: return L("bicycle_surface_any")
      case .preferPaved: return L("bicycle_surface_prefer_paved")
      case .preferUnpaved: return L("bicycle_surface_prefer_unpaved")
      case .avoidUnpaved: return L("avoid_unpaved")
      @unknown default: preconditionFailure("Unknown bicycle surface preference")
      }
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
    case .steps: return \.avoidSteps
    case .difficultTrails: return \.avoidDifficultTrails
    case .bicycleSurface: preconditionFailure("Surface preference uses a selection")
    }
  }
}

struct RoutingOptionsSettingsState {
  let options: RoutingOptions
  let canChangeOptimization: Bool
  let bicycleSurface: BicycleSurfacePreference
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
