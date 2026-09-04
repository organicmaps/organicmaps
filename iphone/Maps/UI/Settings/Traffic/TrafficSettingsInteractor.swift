final class TrafficSettingsInteractor {
  var presenter: TrafficSettingsPresenter?

  private let settings: TrafficSettings.Type
  private var state: TrafficSettingsState

  init(settings: TrafficSettings.Type = Settings.self) {
    self.settings = settings
    state = Self.initialState(settings: settings)
  }

  func loadSettings() {
    present(reconfiguredItems: [], animatingDifferences: false)
  }

  func saveSettings() {
    let apiKey = Self.trimmed(state.apiKey)
    guard apiKey != Self.trimmed(settings.trafficApiKey()) else { return }
    guard state.isKeyValid else { return }
    settings.setTrafficApiKey(apiKey)
  }

  private func changeAPIKey(_ apiKey: String) {
    guard state.apiKey != apiKey else { return }
    state.apiKey = apiKey
    updateState(reconfiguredItems: [.apiKey], animatingDifferences: false)
  }

  private func endEditingAPIKey(_ apiKey: String) {
    let apiKey = Self.trimmed(apiKey)
    guard state.apiKey != apiKey else { return }
    state.apiKey = apiKey
    updateState(reconfiguredItems: [.apiKey], animatingDifferences: false)
  }

  private func present(reconfiguredItems: [TrafficSettingsItem],
                       animatingDifferences: Bool) {
    presenter?.present(state,
                       reconfiguredItems: reconfiguredItems,
                       animatingDifferences: animatingDifferences)
  }

  private func updateState(reconfiguredItems: [TrafficSettingsItem],
                           animatingDifferences: Bool) {
    state.isKeyValid = Self.isKeyValid(apiKey: state.apiKey, settings: settings)
    present(reconfiguredItems: reconfiguredItems, animatingDifferences: animatingDifferences)
  }

  private static func initialState(settings: TrafficSettings.Type) -> TrafficSettingsState {
    let apiKey = trimmed(settings.trafficApiKey())
    return TrafficSettingsState(apiKey: apiKey,
                                isKeyValid: isKeyValid(apiKey: apiKey, settings: settings))
  }

  /// An empty key is always valid: it falls back to the built-in traffic source.
  private static func isKeyValid(apiKey: String, settings: TrafficSettings.Type) -> Bool {
    let apiKey = trimmed(apiKey)
    return apiKey.isEmpty || settings.isWellFormedTrafficApiKey(apiKey)
  }

  private static func trimmed(_ apiKey: String?) -> String {
    (apiKey ?? "").trimmingCharacters(in: .whitespacesAndNewlines)
  }
}

extension TrafficSettingsInteractor: SettingsViewControllerInteractor {
  typealias Section = TrafficSettingsSection
  typealias Item = TrafficSettingsItem

  func handle(_ action: SettingsViewControllerAction<TrafficSettingsItem>) {
    switch action {
    case .didLoad:
      loadSettings()
    case .willDisappear:
      saveSettings()
    case .didChangeText(.apiKey, text: let text):
      changeAPIKey(text)
    case .didEndEditingText(.apiKey, text: let text):
      endEditingAPIKey(text)
    default:
      break
    }
  }
}
