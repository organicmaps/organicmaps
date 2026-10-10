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
      SettingsSectionViewModel(section: .distance,
                               items: Units.settingsOptions.map {
                                 SettingsItemViewModel(item: .distance($0),
                                                       title: $0.title,
                                                       kind: .selectable(isSelected: $0 == state.units))
                               }),
      SettingsSectionViewModel(section: .altitude,
                               header: L("altitude_units"),
                               items: Units.settingsOptions.map {
                                 SettingsItemViewModel(item: .altitude($0),
                                                       title: $0.altitudeTitle,
                                                       kind: .selectable(isSelected: $0 == state.altitudeUnits))
                               }),
    ]
  }
}
