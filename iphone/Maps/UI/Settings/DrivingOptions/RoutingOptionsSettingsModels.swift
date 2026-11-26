enum RoutingOptionsSettingsSection: String {
  case options
  case bicycleSurface
  case transitModes
  case transitPreference
  case optimization
}

extension TransitModes: Hashable {}

enum RoutingOption: Hashable {
  case routeOptimization
  case tollRoads
  case unpavedRoads
  case ferryCrossings
  case motorways
  case steps
  case difficultTrails
  case bicycleSurface(BicycleSurfacePreference)
  case transitMode(TransitModes)
  case transitPreference(TransitPreference)
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
    case .transitMode, .transitPreference: return options.routerType == .publicTransport
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
    case .transitMode(let mode):
      switch mode {
      case .bus: return L("transit_mode_bus")
      case .tram: return L("transit_mode_tram")
      case .subway: return L("transit_mode_subway")
      case .rail: return L("transit_mode_rail")
      case .ferry: return L("transit_mode_ferry")
      case .trolleybus: return L("transit_mode_trolleybus")
      case .monorail: return L("transit_mode_monorail")
      case .cable: return L("transit_mode_cable")
      case .air: return L("transit_mode_air")
      default: preconditionFailure("Expected one transit mode")
      }
    case .transitPreference(let preference):
      switch preference {
      case .recommended: return L("transit_preference_recommended")
      case .lessWalking: return L("transit_preference_less_walking")
      case .fewerTransfers: return L("transit_preference_fewer_transfers")
      @unknown default: preconditionFailure("Unknown transit preference")
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
    case .bicycleSurface, .transitPreference: preconditionFailure("Preferences use a selection")
    case .transitMode: preconditionFailure("Transit modes use the transit mode mask")
    }
  }
}

struct RoutingOptionsSettingsState {
  let options: RoutingOptions
  let canChangeOptimization: Bool
  let bicycleSurface: BicycleSurfacePreference
  let transitModes: TransitModes
  let transitPreference: TransitPreference
}

typealias RoutingOptionsSettingsViewController = SettingsViewController<RoutingOptionsSettingsSection, RoutingOption>
typealias RoutingOptionsSettingsSectionViewModel = SettingsSectionViewModel<RoutingOptionsSettingsSection, RoutingOption>
typealias RoutingOptionsSettingsItemViewModel = SettingsItemViewModel<RoutingOption>

extension MWMRouterType {
  var hasRoutingSettings: Bool {
    self != .ruler
  }

  var routingOptionsTitle: String {
    switch self {
    case .vehicle: return L("routing_options_driving")
    case .bicycle: return L("routing_options_cycling")
    case .pedestrian: return L("routing_options_walking")
    case .publicTransport: return L("public_transport")
    default: return L("driving_options_title")
    }
  }
}
