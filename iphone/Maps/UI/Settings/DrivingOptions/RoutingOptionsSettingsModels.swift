enum RoutingOptionsSettingsSection: String {
  case optimization
  case options
}

extension RoutingOptionsSettingsSection {
  var footer: String? {
    switch self {
    case .optimization: return L("route_optimization_description")
    case .options: return nil
    }
  }
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
}

typealias RoutingOptionsSettingsViewController = SettingsViewController<RoutingOptionsSettingsSection, RoutingOption>
typealias RoutingOptionsSettingsSectionViewModel = SettingsSectionViewModel<RoutingOptionsSettingsSection, RoutingOption>
typealias RoutingOptionsSettingsItemViewModel = SettingsItemViewModel<RoutingOption>
