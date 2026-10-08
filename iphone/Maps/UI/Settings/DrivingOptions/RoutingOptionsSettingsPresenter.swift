final class RoutingOptionsSettingsPresenter {
  private weak var viewController: RoutingOptionsSettingsViewController?

  init(viewController: RoutingOptionsSettingsViewController) {
    self.viewController = viewController
  }

  func present(_ state: RoutingOptionsSettingsState,
               animatingDifferences: Bool = true) {
    viewController?.display(SettingsViewModel(title: state.options.routerType.routingOptionsTitle,
                                              sections: sections(from: state),
                                              animatingDifferences: animatingDifferences))
  }

  func sections(from state: RoutingOptionsSettingsState) -> [RoutingOptionsSettingsSectionViewModel] {
    let avoidances = RoutingOption.avoidanceOptions.filter { $0.isSupported(in: state.options) }
    var sections: [RoutingOptionsSettingsSectionViewModel] = []
    if !avoidances.isEmpty {
      sections.append(SettingsSectionViewModel(section: .options,
                                               items: avoidances.map { item($0, state: state) }))
    }
    sections.append(SettingsSectionViewModel(section: .optimization,
                                             header: L("routing_options_all_modes"),
                                             footer: L("route_optimization_description"),
                                             items: [item(.routeOptimization, state: state)]))
    return sections
  }

  private func item(_ option: RoutingOption,
                    state: RoutingOptionsSettingsState) -> RoutingOptionsSettingsItemViewModel {
    SettingsItemViewModel(item: option,
                          title: option.title,
                          kind: .switcher(isOn: option.isEnabled(in: state.options),
                                          isEnabled: option != .routeOptimization || state.canChangeOptimization))
  }
}
