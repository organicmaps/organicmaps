final class RoutingOptionsSettingsPresenter {
  private weak var viewController: RoutingOptionsSettingsViewController?

  init(viewController: RoutingOptionsSettingsViewController) {
    self.viewController = viewController
  }

  func present(_ state: RoutingOptionsSettingsState,
               animatingDifferences: Bool = true) {
    viewController?.display(SettingsViewModel(title: RootSettings.routingOptions.title,
                                              sections: sections(from: state),
                                              animatingDifferences: animatingDifferences))
  }

  private func sections(from state: RoutingOptionsSettingsState) -> [RoutingOptionsSettingsSectionViewModel] {
    [
      SettingsSectionViewModel(section: .options,
                               items: RoutingOption.avoidanceOptions.map { item($0, state: state) }),
      SettingsSectionViewModel(section: .optimization,
                               footer: RoutingOptionsSettingsSection.optimization.footer,
                               items: [item(.routeOptimization, state: state)]),
    ]
  }

  private func item(_ option: RoutingOption,
                    state: RoutingOptionsSettingsState) -> RoutingOptionsSettingsItemViewModel {
    let isOn = option == .routeOptimization ? state.routeOptimizationEnabled : option.isEnabled(in: state.options)
    return SettingsItemViewModel(item: option,
                                 title: option.title,
                                 kind: .switcher(isOn: isOn,
                                                 isEnabled: option != .routeOptimization || state.canChangeOptimization))
  }
}
