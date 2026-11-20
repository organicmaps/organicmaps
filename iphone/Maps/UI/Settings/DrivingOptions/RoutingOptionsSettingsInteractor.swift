final class RoutingOptionsSettingsInteractor {
  var presenter: RoutingOptionsSettingsPresenter?

  private let routingOptionsProvider: () -> RoutingOptions
  private var state: RoutingOptionsSettingsState?
  private var optimizationEnabledOnLoad = false

  init(routerType: MWMRouterType, routingOptionsProvider: (() -> RoutingOptions)? = nil) {
    self.routingOptionsProvider = routingOptionsProvider ?? { RoutingOptions(routerType: routerType) }
  }

  /// The router type outlives a closed Ruler route, so only an active one blocks the switch.
  private var canChangeOptimization: Bool {
    !MWMRouter.isOnRoute() && (!MWMRouter.isRoutingActive() || MWMRouter.type() != .ruler)
  }

  private func makeState(options: RoutingOptions) -> RoutingOptionsSettingsState {
    let bicycleSurface = options.routerType == .bicycle ? RoutingOptions.bicycleSurfacePreference : .any
    return RoutingOptionsSettingsState(options: options,
                                       canChangeOptimization: canChangeOptimization,
                                       bicycleSurface: bicycleSurface)
  }

  func loadSettings() {
    let state = makeState(options: routingOptionsProvider())
    optimizationEnabledOnLoad = state.options.routeOptimizationEnabled
    self.state = state
    present(state, animatingDifferences: false)
  }

  private func set(_ option: RoutingOption, enabled: Bool) {
    guard state != nil else { return }
    // CarPlay can change avoidance options while this screen is open. Preserve those changes when saving.
    let options = routingOptionsProvider()
    // Navigation can start while the screen is open (e.g. from CarPlay): recheck it and refresh the switch.
    let state = makeState(options: options)
    if option != .routeOptimization || state.canChangeOptimization {
      option.setEnabled(enabled, in: options)
      // Optimization persists in its setter; only avoidance options need saving.
      if option != .routeOptimization {
        options.save()
      }
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
    case .didSelect(.bicycleSurface(let preference)):
      guard let state else { return }
      precondition(state.options.routerType == .bicycle)
      RoutingOptions.bicycleSurfacePreference = preference
      let updated = makeState(options: routingOptionsProvider())
      self.state = updated
      present(updated, animatingDifferences: false)
    case .didDisappear:
      applyOptimization()
    default:
      break
    }
  }
}
