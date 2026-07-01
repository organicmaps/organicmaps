final class UnitsSettingsPresenter {
  private weak var viewController: UnitsSettingsViewController?

  init(viewController: UnitsSettingsViewController) {
    self.viewController = viewController
  }

  func present(_ state: UnitsSettingsState,
               animatingDifferences: Bool = true) {
    viewController?.display(SettingsViewModel(title: RootSettings.units.title,
                                              sections: sections(from: state),
                                              animatingDifferences: animatingDifferences))
  }

  private func sections(from state: UnitsSettingsState) -> [UnitsSettingsSectionViewModel] {
    [
      SettingsSectionViewModel(section: .options,
                               items: Units.settingsOptions.map { item($0, state: state) }),
    ]
  }

  private func item(_ units: Units, state: UnitsSettingsState) -> UnitsSettingsItemViewModel {
    SettingsItemViewModel(item: units,
                          title: units.title,
                          kind: .selectable(isSelected: units == state.units))
  }
}
