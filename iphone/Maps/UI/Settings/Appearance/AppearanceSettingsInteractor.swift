final class AppearanceSettingsInteractor {
  var presenter: AppearanceSettingsPresenter?

  private let settings: Settings.Type

  init(settings: Settings.Type = Settings.self) {
    self.settings = settings
  }

  func loadSettings() {
    present(AppearanceSettingsState(theme: settings.theme().settingsTheme),
            animatingDifferences: false)
  }

  private func select(_ theme: MWMTheme) {
    settings.setTheme(theme)
    present(AppearanceSettingsState(theme: theme), animatingDifferences: false)
  }

  private func present(_ state: AppearanceSettingsState, animatingDifferences: Bool = true) {
    presenter?.present(state, animatingDifferences: animatingDifferences)
  }
}

extension AppearanceSettingsInteractor: SettingsViewControllerInteractor {
  typealias Section = AppearanceSettingsSection
  typealias Item = MWMTheme

  func handle(_ action: SettingsViewControllerAction<MWMTheme>) {
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
