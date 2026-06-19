final class RootSettingsPresenter {
  private weak var viewController: RootSettingsViewController?
  weak var interactor: RootSettingsInteractor?

  init(viewController: RootSettingsViewController) {
    self.viewController = viewController
  }

  func presentSettings(_ state: RootSettingsState,
                       reconfiguredItems: [RootSettings] = [],
                       animatingDifferences: Bool = true) {
    viewController?.display(SettingsViewModel(title: L("settings"),
                                              sections: buildSections(from: state),
                                              reconfiguredItems: reconfiguredItems,
                                              animatingDifferences: animatingDifferences))
  }

  func present(_ screen: SettingsScreen) {
    viewController?.display(screen)
  }

  func present3dBuildingsDisabledAlert() {
    let alert = UIAlertController(title: L("pref_map_3d_buildings_title"),
                                  message: L("pref_map_3d_buildings_disabled_summary"),
                                  preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: "OK", style: .default))
    viewController?.display(alert)
  }

  func presentICloudSynchronizationEnablingAlert(canBackup: Bool) {
    let alert = UIAlertController(title: L("enable_icloud_synchronization_title"),
                                  message: L("enable_icloud_synchronization_message"),
                                  preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: L("cancel"), style: .cancel) { [weak self] _ in
      self?.interactor?.cancelICloudSynchronization()
    })
    if canBackup {
      alert.addAction(UIAlertAction(title: L("backup"), style: .default) { [weak self] _ in
        self?.interactor?.backupBeforeICloudSynchronization()
      })
    }
    alert.addAction(UIAlertAction(title: L("enable"), style: .default) { [weak self] _ in
      self?.interactor?.confirmICloudSynchronization()
    })
    viewController?.display(alert)
  }

  func presentICloudDisabledAlert() {
    let alert = UIAlertController(title: L("icloud_disabled_title"),
                                  message: L("icloud_disabled_message"),
                                  preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: L("ok"), style: .cancel))
    viewController?.display(alert)
  }

  func presentBookmarkBackupShare(url: URL) {
    viewController?.displayBookmarkBackupShare(message: L("share_bookmarks_email_body"),
                                               url: url)
  }

  func presentBookmarkBackupEmptyError() {
    let alert = UIAlertController(title: L("bookmarks_error_title_share_empty"),
                                  message: nil,
                                  preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: L("ok"), style: .default))
    viewController?.display(alert)
  }

  func presentBookmarkBackupError() {
    let alert = UIAlertController(title: L("dialog_routing_system_error"),
                                  message: nil,
                                  preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: L("ok"), style: .default))
    viewController?.display(alert)
  }

  func presentHighlight(_ setting: RootSettings) {
    viewController?.displayHighlight(setting)
  }

  private func buildSections(from state: RootSettingsState) -> [RootSettingsSectionViewModel] {
    [
      SettingsSectionViewModel(section: .profile,
                               items: [profileItem(state)]),
      SettingsSectionViewModel(section: .general,
                               items: generalItems(state)),
      SettingsSectionViewModel(section: .navigation,
                               items: navigationItems(state)),
    ]
  }

  private func profileItem(_ state: RootSettingsState) -> RootSettingsItemViewModel {
    SettingsItemViewModel(setting: .profile,
                          detail: state.osmUserName,
                          kind: .link)
  }

  private func generalItems(_ state: RootSettingsState) -> [RootSettingsItemViewModel] {
    [
      SettingsItemViewModel(setting: .units,
                            detail: state.measurementUnits.title,
                            kind: .link),
      SettingsItemViewModel(setting: .zoomButtons,
                            detail: nil,
                            kind: .switcher(isOn: state.zoomButtonsEnabled, isEnabled: true)),
      SettingsItemViewModel(setting: .buildings3D,
                            detail: nil,
                            kind: .switcher(isOn: state.buildings3DEditable && state.buildings3DEnabled,
                                            isEnabled: state.buildings3DEditable)),
      SettingsItemViewModel(setting: .autoDownload,
                            detail: nil,
                            kind: .switcher(isOn: state.autoDownloadEnabled, isEnabled: true)),
      SettingsItemViewModel(setting: .showDownloadedRegions,
                            detail: nil,
                            kind: .switcher(isOn: state.showDownloadedRegions, isEnabled: true)),
      SettingsItemViewModel(setting: .mobileInternet,
                            detail: MobileInternetSetting(permission: state.mobileInternetPermission).title,
                            kind: .link),
      SettingsItemViewModel(setting: .powerManagement,
                            detail: state.powerManagement.title,
                            kind: .link),
      SettingsItemViewModel(setting: .bookmarksTextPlacement,
                            detail: state.bookmarksTextPlacement.title,
                            kind: .link),
      SettingsItemViewModel(setting: .largeFont,
                            kind: .switcher(isOn: state.largeFontSize, isEnabled: true)),
      SettingsItemViewModel(setting: .transliteration,
                            kind: .switcher(isOn: state.transliteration, isEnabled: true)),
      SettingsItemViewModel(setting: .compassCalibration,
                            kind: .switcher(isOn: state.compassCalibrationEnabled, isEnabled: true)),
      SettingsItemViewModel(setting: .appearance,
                            detail: state.theme.title,
                            kind: .link),
      SettingsItemViewModel(setting: .iCloud,
                            kind: iCloudItemKind(state)),
      SettingsItemViewModel(setting: .mapTiles,
                            kind: .link),
      SettingsItemViewModel(setting: .logging,
                            detail: logFileDetail(state.logFileSize),
                            kind: .switcher(isOn: state.fileLoggingEnabled, isEnabled: true)),
    ]
  }

  private func navigationItems(_ state: RootSettingsState) -> [RootSettingsItemViewModel] {
    [
      SettingsItemViewModel(setting: .perspectiveView,
                            kind: .switcher(isOn: state.map3DEnabled, isEnabled: true)),
      SettingsItemViewModel(setting: .autoZoom,
                            kind: .switcher(isOn: state.autoZoomEnabled, isEnabled: true)),
      SettingsItemViewModel(setting: .voiceInstructions,
                            detail: state.ttsEnabled ? L("on") : L("off"),
                            kind: .link),
      SettingsItemViewModel(setting: .routingOptions,
                            kind: .link),
    ]
  }

  private func iCloudItemKind(_ state: RootSettingsState) -> SettingsItemKind {
    .iCloudSwitcher(isOn: state.iCloudSynchronizationEnabled,
                    isAvailable: state.iCloudSynchronizationState?.isAvailable ?? true,
                    errorDescription: state.iCloudSynchronizationState?.error?.localizedDescription)
  }

  private func logFileDetail(_ size: UInt64) -> String? {
    size == 0 ? nil : String(format: L("log_file_size"), formattedSize(size))
  }
}
