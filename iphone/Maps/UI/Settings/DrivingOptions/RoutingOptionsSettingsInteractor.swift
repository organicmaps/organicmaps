final class RoutingOptionsSettingsInteractor {
  var presenter: RoutingOptionsSettingsPresenter?

  private let routingOptionsProvider: () -> RoutingOptions
  private var state: RoutingOptionsSettingsState?
  private var optimizationEnabledOnLoad = false

  init(routingOptionsProvider: @escaping () -> RoutingOptions = RoutingOptions.init) {
    self.routingOptionsProvider = routingOptionsProvider
  }

  /// The router type outlives a closed Ruler route, so only an active one blocks the switch.
  private var canChangeOptimization: Bool {
    !MWMRouter.isOnRoute() && (!MWMRouter.isRoutingActive() || MWMRouter.type() != .ruler)
  }

  func loadSettings() {
    let state = RoutingOptionsSettingsState(options: routingOptionsProvider(),
                                            canChangeOptimization: canChangeOptimization)
    optimizationEnabledOnLoad = state.options.routeOptimizationEnabled
    self.state = state
    present(state, animatingDifferences: false)
  }

  private func set(_ option: RoutingOption, enabled: Bool) {
    guard let options = state?.options else { return }
    // Navigation can start while the screen is open (e.g. from CarPlay): recheck it and refresh the switch.
    let state = RoutingOptionsSettingsState(options: options, canChangeOptimization: canChangeOptimization)
    if option != .routeOptimization || state.canChangeOptimization {
      option.setEnabled(enabled, in: options)
      options.save()
    }
    self.state = state
    present(state, animatingDifferences: false)
  }

  /// Optimizes the route once after leaving the screen, if the final selection turned optimization on.
  private func applyOptimization() {
    guard let options = state?.options, options.routeOptimizationEnabled, !optimizationEnabledOnLoad else { return }
    MWMRouter.optimizeRoutePointsAndRebuild()
  }

  private func present(_ state: RoutingOptionsSettingsState, animatingDifferences: Bool = true) {
    presenter?.present(state, animatingDifferences: animatingDifferences)
  }
}

extension RoutingOptionsSettingsInteractor: SettingsViewControllerInteractor {
  typealias Section = RoutingOptionsSettingsSection
  typealias Item = RoutingOption

  func handle(_ action: SettingsViewControllerAction<RoutingOption>) {
    switch action {
    case .didLoad:
      loadSettings()
    case .didChangeSwitch(let item, isOn: let isOn):
      set(item, enabled: isOn)
    case .didDisappear:
      applyOptimization()
    default:
      break
    }
  }
}
