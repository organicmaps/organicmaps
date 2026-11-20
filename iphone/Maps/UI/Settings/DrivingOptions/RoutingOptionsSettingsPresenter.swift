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
    if state.options.routerType == .bicycle {
      let preferences: [BicycleSurfacePreference] = [.any, .preferPaved, .preferUnpaved, .avoidUnpaved]
      let items = preferences.map { preference in
        let option = RoutingOption.bicycleSurface(preference)
        return RoutingOptionsSettingsItemViewModel(item: option,
                                                   title: option.title,
                                                   kind: .selectable(isSelected: state.bicycleSurface == preference))
      }
      sections.append(SettingsSectionViewModel(section: .bicycleSurface, header: L("bicycle_surface"), items: items))
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
