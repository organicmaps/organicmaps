final class RoutingOptionsSettingsInteractor {
  var presenter: RoutingOptionsSettingsPresenter?

  private let routingOptionsProvider: () -> RoutingOptions
  private var state: RoutingOptionsSettingsState?

  init(routingOptionsProvider: @escaping () -> RoutingOptions = RoutingOptions.init) {
    self.routingOptionsProvider = routingOptionsProvider
  }

  func loadSettings() {
    let state = RoutingOptionsSettingsState(options: routingOptionsProvider())
    self.state = state
    present(state, animatingDifferences: false)
  }

  private func set(_ option: RoutingOption, enabled: Bool) {
    guard let state else { return }
    option.setEnabled(enabled, in: state.options)
    state.options.save()
    present(state, animatingDifferences: false)
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
    default:
      break
    }
  }
}
