final class PowerManagementSettingsInteractor {
  var presenter: PowerManagementSettingsPresenter?

  private let settings: Settings.Type

  init(settings: Settings.Type = Settings.self) {
    self.settings = settings
  }

  func loadSettings() {
    present(PowerManagementSettingsState(powerManagement: settings.powerManagement()),
            animatingDifferences: false)
  }

  private func select(_ powerManagement: MWMSettingsPowerManagement) {
    settings.setPowerManagement(powerManagement)
    present(PowerManagementSettingsState(powerManagement: powerManagement),
            animatingDifferences: false)
  }

  private func present(_ state: PowerManagementSettingsState, animatingDifferences: Bool = true) {
    presenter?.present(state, animatingDifferences: animatingDifferences)
  }
}

extension PowerManagementSettingsInteractor: SettingsViewControllerInteractor {
  typealias Section = PowerManagementSettingsSection
  typealias Item = MWMSettingsPowerManagement

  func handle(_ action: SettingsViewControllerAction<MWMSettingsPowerManagement>) {
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
