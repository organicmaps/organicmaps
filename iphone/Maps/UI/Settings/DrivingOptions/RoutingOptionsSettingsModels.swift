enum RoutingOptionsSettingsSection: String {
  case options
}

enum RoutingOption: String, CaseIterable {
  case tollRoads
  case unpavedRoads
  case ferryCrossings
  case motorways
}

extension RoutingOption {
  var title: String {
    switch self {
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
