final class MapTilesSettingsInteractor {
  var presenter: MapTilesSettingsPresenter?

  private let settings: Settings.Type
  private var state: MapTilesSettingsState

  init(settings: Settings.Type = Settings.self) {
    self.settings = settings
    state = Self.initialState(settings: settings)
  }

  func loadSettings() {
    present(reconfiguredItems: [], animatingDifferences: false)
  }

  func saveSettings() {
    settings.setBackgroundTiles(enabled: state.isEnabled && state.isConfigValid,
                                url: Self.trimmed(state.url),
                                cacheSizeMB: state.cacheSizeMB,
                                areaOpacityPct: state.opacityPct)
  }

  private func setEnabled(_ isEnabled: Bool) {
    guard state.isEnabled != isEnabled else { return }
    state.isEnabled = isEnabled
    state.url = Self.trimmed(state.url)
    updateState(reconfiguredItems: [.url], animatingDifferences: false)
  }

  private func changeURL(_ url: String) {
    guard state.url != url else { return }
    state.url = url
    updateState(reconfiguredItems: [.url], animatingDifferences: false)
  }

  private func endEditingURL(_ url: String) {
    let url = Self.trimmed(url)
    guard state.url != url else { return }
    state.url = url
    updateState(reconfiguredItems: [.url], animatingDifferences: false)
  }

  private func setCacheSize(_ value: Float) {
    let cacheSizeMB = Self.clamped(Int(value.rounded()),
                                   min: MapTilesSettingsLimits.minCacheSizeMB,
                                   max: MapTilesSettingsLimits.maxCacheSizeMB)
    guard state.cacheSizeMB != cacheSizeMB else { return }
    state.cacheSizeMB = cacheSizeMB
    updateState(reconfiguredItems: [.cacheSize], animatingDifferences: false)
  }

  private func setOpacity(_ value: Float) {
    let opacityPct = Self.clamped(Int(value.rounded()),
                                  min: MapTilesSettingsLimits.minOpacityPct,
                                  max: MapTilesSettingsLimits.maxOpacityPct)
    guard state.opacityPct != opacityPct else { return }
    state.opacityPct = opacityPct
    updateState(reconfiguredItems: [.opacity], animatingDifferences: false)
  }

  private func present(reconfiguredItems: [MapTilesSettingsItem],
                       animatingDifferences: Bool) {
    presenter?.present(state,
                       reconfiguredItems: reconfiguredItems,
                       animatingDifferences: animatingDifferences)
  }

  private func updateState(reconfiguredItems: [MapTilesSettingsItem],
                           animatingDifferences: Bool) {
    state.isConfigValid = Self.isConfigValid(isEnabled: state.isEnabled, url: state.url, settings: settings)
    present(reconfiguredItems: reconfiguredItems, animatingDifferences: animatingDifferences)
  }

  private static func initialState(settings: Settings.Type) -> MapTilesSettingsState {
    let isEnabled = settings.backgroundTilesEnabled()
    let url = trimmed(settings.backgroundTilesURL())
    return MapTilesSettingsState(isEnabled: isEnabled,
                                 url: url,
                                 cacheSizeMB: clamped(settings.backgroundTilesCacheSizeMB(),
                                                      min: MapTilesSettingsLimits.minCacheSizeMB,
                                                      max: MapTilesSettingsLimits.maxCacheSizeMB),
                                 opacityPct: clamped(settings.backgroundTilesAreaOpacityPct(),
                                                     min: MapTilesSettingsLimits.minOpacityPct,
                                                     max: MapTilesSettingsLimits.maxOpacityPct),
                                 isConfigValid: isConfigValid(isEnabled: isEnabled, url: url, settings: settings))
  }

  private static func isConfigValid(isEnabled: Bool, url: String, settings: Settings.Type) -> Bool {
    !isEnabled || settings.isWellFormedBackgroundTilesURL(trimmed(url))
  }

  private static func clamped(_ value: Int, min: Int, max: Int) -> Int {
    Swift.min(Swift.max(value, min), max)
  }

  private static func trimmed(_ url: String?) -> String {
    (url ?? "").trimmingCharacters(in: .whitespacesAndNewlines)
  }
}

extension MapTilesSettingsInteractor: SettingsViewControllerInteractor {
  typealias Section = MapTilesSettingsSection
  typealias Item = MapTilesSettingsItem

  func handle(_ action: SettingsViewControllerAction<MapTilesSettingsItem>) {
    switch action {
    case .didLoad:
      loadSettings()
    case .willDisappear:
      saveSettings()
    case .didChangeSwitch(.enable, isOn: let isOn):
      setEnabled(isOn)
    case .didChangeText(.url, text: let text):
      changeURL(text)
    case .didEndEditingText(.url, text: let text):
      endEditingURL(text)
    case .didChangeSlider(let item, value: let value):
      changeSlider(item, value: value)
    default:
      break
    }
  }

  private func changeSlider(_ item: MapTilesSettingsItem, value: Float) {
    switch item {
    case .cacheSize:
      setCacheSize(value)
    case .opacity:
      setOpacity(value)
    case .enable, .url, .urlError:
      return
    }
  }
}
