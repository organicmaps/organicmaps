final class PowerManagementSettingsPresenter {
  private weak var viewController: PowerManagementSettingsViewController?

  init(viewController: PowerManagementSettingsViewController) {
    self.viewController = viewController
  }

  func present(_ state: PowerManagementSettingsState,
               animatingDifferences: Bool = true) {
    viewController?.display(SettingsViewModel(title: RootSettings.powerManagement.title,
                                              sections: sections(from: state),
                                              animatingDifferences: animatingDifferences))
  }

  private func sections(from state: PowerManagementSettingsState) -> [PowerManagementSettingsSectionViewModel] {
    [
      SettingsSectionViewModel(section: .options,
                               footer: PowerManagementSettingsSection.options.footer,
                               items: MWMSettingsPowerManagement.settingsOptions.map { item($0, state: state) }),
    ]
  }

  private func item(_ powerManagement: MWMSettingsPowerManagement,
                    state: PowerManagementSettingsState) -> PowerManagementSettingsItemViewModel {
    SettingsItemViewModel(item: powerManagement,
                          title: powerManagement.title,
                          kind: .selectable(isSelected: powerManagement == state.powerManagement))
  }
}
