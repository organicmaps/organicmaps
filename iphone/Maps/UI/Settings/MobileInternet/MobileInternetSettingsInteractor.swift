final class MobileInternetSettingsInteractor {
  var presenter: MobileInternetSettingsPresenter?

  private let settings: Settings.Type

  init(settings: Settings.Type = Settings.self) {
    self.settings = settings
  }

  func loadSettings() {
    present(MobileInternetSettingsState(permission: settings.mobileInternetPermission()),
            animatingDifferences: false)
  }

  private func select(_ setting: MobileInternetSetting) {
    settings.setMobileInternetPermission(setting.permission)
    present(MobileInternetSettingsState(permission: setting.permission),
            animatingDifferences: false)
  }

  private func present(_ state: MobileInternetSettingsState, animatingDifferences: Bool = true) {
    presenter?.present(state, animatingDifferences: animatingDifferences)
  }
}

extension MobileInternetSettingsInteractor: SettingsViewControllerInteractor {
  typealias Section = MobileInternetSettingsSection
  typealias Item = MobileInternetSetting

  func handle(_ action: SettingsViewControllerAction<MobileInternetSetting>) {
    switch action {
    case .didLoad:
      loadSettings()
    case .didSelect(let item):
      select(item)
    default:
      break
    }
  }
}
