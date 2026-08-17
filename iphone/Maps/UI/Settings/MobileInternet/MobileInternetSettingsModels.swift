enum MobileInternetSettingsSection: String {
  case options
}

extension MobileInternetSettingsSection {
  var footer: String? {
    switch self {
    case .options: return L("mobile_data_description")
    }
  }
}

enum MobileInternetSetting: String, CaseIterable {
  case always
  case ask
  case never
}

extension MobileInternetSetting {
  init(permission: MWMNetworkPolicyPermission) {
    switch permission {
    case .always:
      self = .always
    case .never:
      self = .never
    case .ask, .today, .notToday:
      self = .ask
    @unknown default:
      self = .ask
    }
  }

  var title: String {
    switch self {
    case .always: return L("mobile_data_option_always")
    case .ask: return L("mobile_data_option_ask")
    case .never: return L("mobile_data_option_never")
    }
  }

  var permission: MWMNetworkPolicyPermission {
    switch self {
    case .always: return .always
    case .ask: return .ask
    case .never: return .never
    }
  }

  func isSelected(for permission: MWMNetworkPolicyPermission) -> Bool {
    self == MobileInternetSetting(permission: permission)
  }
}

struct MobileInternetSettingsState: Equatable {
  var permission: MWMNetworkPolicyPermission
}

typealias MobileInternetSettingsViewController = SettingsViewController<MobileInternetSettingsSection, MobileInternetSetting>
typealias MobileInternetSettingsSectionViewModel = SettingsSectionViewModel<MobileInternetSettingsSection, MobileInternetSetting>
typealias MobileInternetSettingsItemViewModel = SettingsItemViewModel<MobileInternetSetting>
