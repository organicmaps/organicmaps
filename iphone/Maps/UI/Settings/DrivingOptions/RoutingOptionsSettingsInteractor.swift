final class RoutingOptionsSettingsInteractor {
  var presenter: RoutingOptionsSettingsPresenter?

  private let routingOptionsProvider: () -> RoutingOptions
  private var state: RoutingOptionsSettingsState?

  init(routingOptionsProvider: @escaping () -> RoutingOptions = RoutingOptions.init) {
    self.routingOptionsProvider = routingOptionsProvider
  }

  func loadSettings() {
    let options = routingOptionsProvider()
    let state = RoutingOptionsSettingsState(options: options,
                                            canChangeOptimization: !MWMRouter.isOnRoute(),
                                            routeOptimizationEnabled: options.routeOptimizationEnabled)
    self.state = state
    present(state, animatingDifferences: false)
  }

  private func set(_ option: RoutingOption, enabled: Bool) {
    guard var state else { return }
    guard option != .routeOptimization || !MWMRouter.isOnRoute() else { return }
    if option == .routeOptimization {
      state.routeOptimizationEnabled = enabled
    } else {
      option.setEnabled(enabled, in: state.options)
      state.options.save()
    }
    self.state = state
    present(state, animatingDifferences: false)
  }

  private func applyOptimization() {
    guard let state, state.canChangeOptimization, !MWMRouter.isOnRoute(),
          state.routeOptimizationEnabled != state.options.routeOptimizationEnabled else { return }
    state.options.routeOptimizationEnabled = state.routeOptimizationEnabled
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
