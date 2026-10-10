final class UnitsSettingsInteractor {
  var presenter: UnitsSettingsPresenter?

  private let settings: Settings.Type

  init(settings: Settings.Type = Settings.self) {
    self.settings = settings
  }

  func loadSettings() {
    present(UnitsSettingsState(units: settings.measurementUnits(), altitudeUnits: settings.altitudeUnits()),
            animatingDifferences: false)
  }

  private func select(_ item: UnitsSettingsItem) {
    switch item {
    case .distance(let units):
      settings.setMeasurementUnits(units)
    case .altitude(let units):
      settings.setAltitudeUnits(units)
    }
    loadSettings()
  }

  private func present(_ state: UnitsSettingsState, animatingDifferences: Bool = true) {
    presenter?.present(state, animatingDifferences: animatingDifferences)
  }
}

extension UnitsSettingsInteractor: SettingsViewControllerInteractor {
  typealias Section = UnitsSettingsSection
  typealias Item = UnitsSettingsItem

  func handle(_ action: SettingsViewControllerAction<UnitsSettingsItem>) {
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
