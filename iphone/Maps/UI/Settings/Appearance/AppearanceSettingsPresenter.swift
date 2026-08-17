final class AppearanceSettingsPresenter {
  private weak var viewController: AppearanceSettingsViewController?

  init(viewController: AppearanceSettingsViewController) {
    self.viewController = viewController
  }

  func present(_ state: AppearanceSettingsState,
               animatingDifferences: Bool = true) {
    viewController?.display(SettingsViewModel(title: RootSettings.appearance.title,
                                              sections: sections(from: state),
                                              animatingDifferences: animatingDifferences))
  }

  private func sections(from state: AppearanceSettingsState) -> [AppearanceSettingsSectionViewModel] {
    [
      SettingsSectionViewModel(section: .options,
                               items: MWMTheme.settingsOptions.map { item($0, state: state) }),
    ]
  }

  private func item(_ theme: MWMTheme, state: AppearanceSettingsState) -> AppearanceSettingsItemViewModel {
    SettingsItemViewModel(item: theme,
                          title: theme.title,
                          kind: .selectable(isSelected: theme == state.theme))
  }
}
