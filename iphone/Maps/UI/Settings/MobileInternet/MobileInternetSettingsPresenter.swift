final class MobileInternetSettingsPresenter {
  private weak var viewController: MobileInternetSettingsViewController?

  init(viewController: MobileInternetSettingsViewController) {
    self.viewController = viewController
  }

  func present(_ state: MobileInternetSettingsState,
               animatingDifferences: Bool = true) {
    viewController?.display(SettingsViewModel(title: RootSettings.mobileInternet.title,
                                              sections: sections(from: state),
                                              animatingDifferences: animatingDifferences))
  }

  private func sections(from state: MobileInternetSettingsState) -> [MobileInternetSettingsSectionViewModel] {
    [
      SettingsSectionViewModel(section: .options,
                               header: nil,
                               footer: MobileInternetSettingsSection.options.footer,
                               items: [
                                 item(.always, state: state),
                                 item(.ask, state: state),
                                 item(.never, state: state),
                               ]),
    ]
  }

  private func item(_ setting: MobileInternetSetting, state: MobileInternetSettingsState) -> MobileInternetSettingsItemViewModel {
    SettingsItemViewModel(item: setting,
                          title: setting.title,
                          kind: .selectable(isSelected: setting.isSelected(for: state.permission)))
  }
}
