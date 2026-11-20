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
      sections.append(selection(section: .bicycleSurface,
                                title: L("bicycle_surface"),
                                options: preferences.map(RoutingOption.bicycleSurface),
                                selected: .bicycleSurface(state.bicycleSurface)))
    }
    if state.options.routerType == .publicTransport {
      let preferences: [TransitPreference] = [.recommended, .lessWalking, .fewerTransfers]
      sections.append(selection(section: .transitPreference,
                                title: L("transit_preference"),
                                options: preferences.map(RoutingOption.transitPreference),
                                selected: .transitPreference(state.transitPreference)))
      let selected = state.transitModes
      let modes: [TransitModes] = [.bus, .tram, .subway, .rail, .ferry, .trolleybus, .monorail, .cable, .air]
      let items = modes.map { mode in
        let option = RoutingOption.transitMode(mode)
        return RoutingOptionsSettingsItemViewModel(item: option,
                                                   title: option.title,
                                                   kind: .switcher(isOn: selected.contains(mode), isEnabled: selected != mode))
      }
      sections.append(SettingsSectionViewModel(section: .transitModes, header: L("transit_mode_options"), items: items))
    }
    sections.append(SettingsSectionViewModel(section: .optimization,
                                             header: L("routing_options_all_modes"),
                                             footer: L("route_optimization_description"),
                                             items: [item(.routeOptimization, state: state)]))
    return sections
  }

  private func selection(section: RoutingOptionsSettingsSection,
                         title: String,
                         options: [RoutingOption],
                         selected: RoutingOption) -> RoutingOptionsSettingsSectionViewModel {
    SettingsSectionViewModel(section: section, header: title, items: options.map { option in
      RoutingOptionsSettingsItemViewModel(item: option, title: option.title, kind: .selectable(isSelected: selected == option))
    })
  }

  private func item(_ option: RoutingOption,
                    state: RoutingOptionsSettingsState) -> RoutingOptionsSettingsItemViewModel {
    SettingsItemViewModel(item: option,
                          title: option.title,
                          kind: .switcher(isOn: option.isEnabled(in: state.options),
                                          isEnabled: option != .routeOptimization || state.canChangeOptimization))
  }
}
