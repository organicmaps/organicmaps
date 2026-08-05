final class RoutingOptionsSettingsInteractor {
  var presenter: RoutingOptionsSettingsPresenter?

  private let routingOptionsProvider: () -> RoutingOptions
  private var state: RoutingOptionsSettingsState?

  init(routingOptionsProvider: @escaping () -> RoutingOptions = RoutingOptions.init) {
    self.routingOptionsProvider = routingOptionsProvider
  }

  func loadSettings() {
    let state = RoutingOptionsSettingsState(options: routingOptionsProvider(),
                                            speedSettings: RouteSpeedSettings.current())
    self.state = state
    present(state, animatingDifferences: false)
  }

  private func set(_ option: RoutingOption, enabled: Bool) {
    guard let state else { return }
    option.setEnabled(enabled, in: state.options)
    state.options.save()
    present(state, animatingDifferences: false)
  }

  private func setCruisingSpeed(_ value: Float) {
    guard var state, let speedSettings = state.speedSettings else { return }
    state.cruisingSpeedKMpH = Self.snap(value, step: speedSettings.speedStepKMpH,
                                        minimum: speedSettings.minimumSpeedKMpH,
                                        maximum: speedSettings.maximumSpeedKMpH)
    self.state = state
    presenter?.present(state, reconfiguredItems: [.routeSpeed], animatingDifferences: false)
  }

  private func setWindEnabled(_ enabled: Bool) {
    guard var state else { return }
    state.windEnabled = enabled
    self.state = state
    present(state)
  }

  private func setWindSpeed(_ value: Float) {
    guard var state, let speedSettings = state.speedSettings else { return }
    state.windSpeedMpS = Int(Self.snap(value, step: 1, minimum: 1, maximum: Double(speedSettings.maximumWindSpeedMpS)))
    self.state = state
    presenter?.present(state, reconfiguredItems: [.windSpeed], animatingDifferences: false)
  }

  private func setWindDirection(_ value: Float) {
    guard var state else { return }
    let step = Double(RouteSpeedSettings.windDirectionStepDegrees)
    state.windDirectionDegrees = Int(Self.snap(value, step: step, minimum: 0, maximum: 360 - step))
    self.state = state
    presenter?.present(state, reconfiguredItems: [.windDirection], animatingDifferences: false)
  }

  /// Sliders are continuous, the settings behind them are not.
  private static func snap(_ value: Float, step: Double, minimum: Double, maximum: Double) -> Double {
    min(max((Double(value) / step).rounded() * step, minimum), maximum)
  }

  private func saveSpeedSettings() {
    guard let state, let speedSettings = state.speedSettings, state.isSpeedChanged else { return }
    // Replacing the core router resets the active routing session, so remember its state first.
    let shouldRebuild = MWMRouter.isRoutingActive()
    speedSettings.cruisingSpeedKMpH = state.cruisingSpeedKMpH
    speedSettings.windSpeedMpS = state.windSpeedToApplyMpS
    speedSettings.windDirectionDegrees = state.windDirectionDegrees
    speedSettings.save()
    if shouldRebuild {
      MWMRouter.rebuild(withBestRouter: false)
    }
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
    case .willDisappear:
      saveSpeedSettings()
    case .didChangeSwitch(.windEnabled, isOn: let isOn):
      setWindEnabled(isOn)
    case .didChangeSwitch(let item, isOn: let isOn):
      set(item, enabled: isOn)
    case .didChangeSlider(.routeSpeed, value: let value):
      setCruisingSpeed(value)
    case .didChangeSlider(.windSpeed, value: let value):
      setWindSpeed(value)
    case .didChangeSlider(.windDirection, value: let value):
      setWindDirection(value)
    default:
      break
    }
  }
}
