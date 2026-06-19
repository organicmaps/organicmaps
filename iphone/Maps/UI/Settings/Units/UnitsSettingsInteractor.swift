final class UnitsSettingsInteractor {
  var presenter: UnitsSettingsPresenter?

  private let settings: Settings.Type

  init(settings: Settings.Type = Settings.self) {
    self.settings = settings
  }

  func loadSettings() {
    present(UnitsSettingsState(units: settings.measurementUnits()),
            animatingDifferences: false)
  }

  private func select(_ units: Units) {
    settings.setMeasurementUnits(units)
    present(UnitsSettingsState(units: units), animatingDifferences: false)
  }

  private func present(_ state: UnitsSettingsState, animatingDifferences: Bool = true) {
    presenter?.present(state, animatingDifferences: animatingDifferences)
  }
}

extension UnitsSettingsInteractor: SettingsViewControllerInteractor {
  typealias Section = UnitsSettingsSection
  typealias Item = Units

  func handle(_ action: SettingsViewControllerAction<Units>) {
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
