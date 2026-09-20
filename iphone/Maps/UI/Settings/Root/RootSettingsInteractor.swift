final class RootSettingsInteractor {
  var presenter: RootSettingsPresenter?

  private let settings: Settings.Type
  private var iCloudSynchronizationState: SynchronizationManagerState?
  private var isObserving = false

  deinit {
    iCloudSynchronizaionManager.shared.removeObserver(self)
  }

  init(settings: Settings.Type = Settings.self) {
    self.settings = settings
  }

  func loadSettings() {
    startObserving()
    updateSettings(animatingDifferences: false)
  }

  func reloadSettings() {
    updateSettings(animatingDifferences: false)
  }

  func highlightInAppFeature() {
    guard let setting = highlightedSetting() else { return }
    presenter?.presentHighlight(setting)
  }

  func select(_ screen: SettingsScreen) {
    presenter?.present(screen)
  }

  func tapAccessory(_ setting: RootSettings) {
    if setting == .iCloud {
      presenter?.presentICloudDisabledAlert()
    }
  }

  func set(_ setting: RootSettings, enabled: Bool) {
    if setting == .buildings3D, !isBuildings3DEditable {
      presenter?.present3dBuildingsDisabledAlert()
      updateSettings(reconfiguredItems: [.buildings3D])
      return
    }

    if setting == .iCloud, !settings.didShowICloudSynchronizationEnablingAlert() {
      presenter?.presentICloudSynchronizationEnablingAlert(canBackup: canBackupBookmarks())
      updateSettings(reconfiguredItems: [.iCloud])
      return
    }

    setSetting(setting, enabled: enabled)
    updateSettings()
  }

  func confirmICloudSynchronization() {
    settings.setICloudSynchronizationEnablingAlertShown()
    setSetting(.iCloud, enabled: true)
    updateSettings(reconfiguredItems: [.iCloud])
  }

  func cancelICloudSynchronization() {
    setSetting(.iCloud, enabled: false)
    updateSettings(reconfiguredItems: [.iCloud])
  }

  func backupBeforeICloudSynchronization() {
    BookmarksManager.shared().shareAllCategories { [weak self] status, url in
      guard let self else { return }
      switch status {
      case .success:
        guard let url else {
          self.presenter?.presentBookmarkBackupError()
          self.cancelICloudSynchronization()
          return
        }
        self.presenter?.presentBookmarkBackupShare(url: url)
      case .emptyCategory:
        self.presenter?.presentBookmarkBackupEmptyError()
        self.cancelICloudSynchronization()
      case .archiveError, .fileError:
        self.presenter?.presentBookmarkBackupError()
        self.cancelICloudSynchronization()
      @unknown default:
        self.presenter?.presentBookmarkBackupError()
        self.cancelICloudSynchronization()
      }
    }
  }

  func completeICloudBackupSharing(completed: Bool) {
    if completed {
      settings.setICloudSynchronizationEnablingAlertShown()
    }
    setSetting(.iCloud, enabled: completed)
    updateSettings(reconfiguredItems: [.iCloud])
  }

  private func startObserving() {
    guard !isObserving else { return }
    isObserving = true
    iCloudSynchronizaionManager.shared.addObserver(self) { [weak self] state in
      self?.iCloudSynchronizationState = state
      self?.updateSettings()
    }
  }

  private func updateSettings(reconfiguredItems: [RootSettings] = [], animatingDifferences: Bool = true) {
    presenter?.presentSettings(loadState(),
                               reconfiguredItems: reconfiguredItems,
                               animatingDifferences: animatingDifferences)
  }

  private func loadState() -> RootSettingsState {
    RootSettingsState(osmUserName: settings.osmUserName() ?? "",
                      measurementUnits: settings.measurementUnits(),
                      zoomButtonsEnabled: settings.zoomButtonsEnabled(),
                      buildings3DEnabled: settings.map3dBuildingsEnabled(),
                      buildings3DEditable: isBuildings3DEditable,
                      autoDownloadEnabled: settings.autoDownloadEnabled(),
                      showDownloadedRegions: settings.isShowDownloadedRegions(),
                      mobileInternetPermission: settings.mobileInternetPermission(),
                      powerManagement: settings.powerManagement(),
                      bookmarksTextPlacement: settings.bookmarksTextPlacement(),
                      largeFontSize: settings.largeFontSize(),
                      transliteration: settings.transliteration(),
                      theme: settings.theme(),
                      iCloudSynchronizationEnabled: settings.iCLoudSynchronizationEnabled(),
                      iCloudSynchronizationState: iCloudSynchronizationState,
                      map3DEnabled: settings.perspectiveViewEnabled(),
                      autoZoomEnabled: settings.autoZoomEnabled(),
                      ttsEnabled: MWMTextToSpeech.isTTSEnabled(),
                      fileLoggingEnabled: settings.isFileLoggingEnabled(),
                      logFileSize: settings.logFileSize(),
                      searchHistoryEnabled: settings.searchHistoryEnabled())
  }

  private var isBuildings3DEditable: Bool {
    !settings.isPowerManagementMaximum()
  }

  private func setSetting(_ setting: RootSettings, enabled: Bool) {
    switch setting {
    case .zoomButtons:
      settings.setZoomButtonsEnabled(enabled)
    case .buildings3D:
      settings.setMap3dBuildingsEnabled(enabled)
    case .autoDownload:
      settings.setAutoDownloadEnabled(enabled)
    case .showDownloadedRegions:
      settings.setShowDownloadedRegions(enabled)
    case .largeFont:
      settings.setLargeFontSize(enabled)
    case .transliteration:
      settings.setTransliteration(enabled)
    case .iCloud:
      settings.setICLoudSynchronizationEnabled(enabled)
    case .logging:
      settings.setFileLoggingEnabled(enabled)
    case .perspectiveView:
      settings.setPerspectiveViewEnabled(enabled)
    case .autoZoom:
      settings.setAutoZoomEnabled(enabled)
    case .searchHistory:
      settings.setSearchHistoryEnabled(enabled)
      if !enabled {
        MWMSearchFrameworkHelper.clearSearchHistory()
      }
    case .profile,
         .units,
         .mobileInternet,
         .powerManagement,
         .bookmarksTextPlacement,
         .appearance,
         .mapTiles,
         .voiceInstructions,
         .routingOptions:
      break
    }
  }

  private func canBackupBookmarks() -> Bool {
    !BookmarksManager.shared().areAllCategoriesEmpty()
  }

  private func highlightedSetting() -> RootSettings? {
    guard let featureToHighlight = DeepLinkHandler.shared.getInAppFeatureHighlightData(),
          featureToHighlight.urlType == .settings else { return nil }
    switch featureToHighlight.feature {
    case .iCloud:
      return .iCloud
    case .none, .trackRecorder:
      return nil
    @unknown default:
      return nil
    }
  }
}

extension RootSettingsInteractor: SettingsViewControllerInteractor {
  typealias Section = RootSettingsSection
  typealias Item = RootSettings

  func handle(_ action: SettingsViewControllerAction<RootSettings>) {
    switch action {
    case .didLoad:
      loadSettings()
    case .willAppear:
      reloadSettings()
    case .didAppear:
      highlightInAppFeature()
    case .didSelect(let setting):
      select(setting)
    case .didTapAccessory(let setting):
      tapAccessory(setting)
    case .didChangeSwitch(let setting, isOn: let isOn):
      set(setting, enabled: isOn)
    case .didCompleteBookmarkBackupSharing(let completed):
      completeICloudBackupSharing(completed: completed)
    default:
      break
    }
  }

  private func select(_ setting: RootSettings) {
    if setting == .buildings3D, !isBuildings3DEditable {
      set(setting, enabled: false)
      return
    }
    guard let screen = setting.screen else { return }
    select(screen)
  }
}
