final class MapTilesSettingsPresenter {
  private weak var viewController: MapTilesSettingsViewController?

  init(viewController: MapTilesSettingsViewController) {
    self.viewController = viewController
  }

  func present(_ state: MapTilesSettingsState,
               reconfiguredItems: [MapTilesSettingsItem] = [],
               animatingDifferences: Bool = true) {
    viewController?.display(SettingsViewModel(title: RootSettings.mapTiles.title,
                                              sections: sections(from: state),
                                              reconfiguredItems: reconfiguredItems,
                                              animatingDifferences: animatingDifferences))
  }

  private func sections(from state: MapTilesSettingsState) -> [MapTilesSettingsSectionViewModel] {
    [
      section(.enable, items: [enableItem(state)]),
      section(.url, items: urlItems(state)),
      section(.cacheSize, items: [cacheSizeItem(state)]),
      section(.opacity, items: [opacityItem(state)]),
    ]
  }

  private func section(_ section: MapTilesSettingsSection,
                       items: [MapTilesSettingsItemViewModel]) -> MapTilesSettingsSectionViewModel {
    SettingsSectionViewModel(section: section, header: section.title, footer: section.footer, items: items)
  }

  private func enableItem(_ state: MapTilesSettingsState) -> MapTilesSettingsItemViewModel {
    SettingsItemViewModel(item: .enable,
                          title: RootSettings.mapTiles.title,
                          kind: .switcher(isOn: state.isEnabled, isEnabled: true))
  }

  private func urlItems(_ state: MapTilesSettingsState) -> [MapTilesSettingsItemViewModel] {
    if state.isConfigValid {
      return [urlItem(state)]
    }
    return [urlItem(state), urlErrorItem()]
  }

  private func urlItem(_ state: MapTilesSettingsState) -> MapTilesSettingsItemViewModel {
    SettingsItemViewModel(item: .url,
                          kind: .textField(text: state.url,
                                           placeholder: "https://xxx.yyy/{z}/{x}/{y}.png",
                                           isEnabled: state.isEnabled,
                                           isValid: state.isConfigValid))
  }

  private func urlErrorItem() -> MapTilesSettingsItemViewModel {
    SettingsItemViewModel(item: .urlError, kind: .message(text: L("pref_bg_tiles_url_error")))
  }

  private func cacheSizeItem(_ state: MapTilesSettingsState) -> MapTilesSettingsItemViewModel {
    SettingsItemViewModel(item: .cacheSize,
                          kind: .slider(value: Float(state.cacheSizeMB),
                                        minimumValue: Float(state.limits.minCacheSizeMB),
                                        maximumValue: Float(state.limits.maxCacheSizeMB),
                                        valueTitle: "\(state.cacheSizeMB)",
                                        isEnabled: state.isEnabled))
  }

  private func opacityItem(_ state: MapTilesSettingsState) -> MapTilesSettingsItemViewModel {
    SettingsItemViewModel(item: .opacity,
                          kind: .slider(value: Float(state.opacityPct),
                                        minimumValue: Float(state.limits.minOpacityPct),
                                        maximumValue: Float(state.limits.maxOpacityPct),
                                        valueTitle: "\(state.opacityPct)%",
                                        isEnabled: state.isEnabled))
  }
}
