final class TrafficSettingsPresenter {
  private weak var viewController: TrafficSettingsViewController?

  init(viewController: TrafficSettingsViewController) {
    self.viewController = viewController
  }

  func present(_ state: TrafficSettingsState,
               reconfiguredItems: [TrafficSettingsItem] = [],
               animatingDifferences: Bool = true) {
    viewController?.display(SettingsViewModel(title: RootSettings.traffic.title,
                                              sections: sections(from: state),
                                              reconfiguredItems: reconfiguredItems,
                                              animatingDifferences: animatingDifferences))
  }

  private func sections(from state: TrafficSettingsState) -> [TrafficSettingsSectionViewModel] {
    [section(.apiKey, items: keyItems(state))]
  }

  private func section(_ section: TrafficSettingsSection,
                       items: [TrafficSettingsItemViewModel]) -> TrafficSettingsSectionViewModel {
    SettingsSectionViewModel(section: section, header: section.title, footer: section.footer, items: items)
  }

  private func keyItems(_ state: TrafficSettingsState) -> [TrafficSettingsItemViewModel] {
    if state.isKeyValid {
      return [keyItem(state)]
    }
    return [keyItem(state), keyErrorItem()]
  }

  private func keyItem(_ state: TrafficSettingsState) -> TrafficSettingsItemViewModel {
    SettingsItemViewModel(item: .apiKey,
                          kind: .textField(text: state.apiKey,
                                           placeholder: "",
                                           isEnabled: true,
                                           isValid: state.isKeyValid))
  }

  private func keyErrorItem() -> TrafficSettingsItemViewModel {
    SettingsItemViewModel(item: .keyError, kind: .message(text: L("pref_traffic_key_error")))
  }
}
