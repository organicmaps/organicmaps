@objc @objcMembers class DeepLinkHandler: NSObject {
  static let shared = DeepLinkHandler()

  private(set) var isLaunchedByDeepLink = false
  private(set) var hasPendingColdLaunchDeepLink = false
  private(set) var url: URL?

  override private init() {
    super.init()
  }

  /// Keeps the last link received during a cold launch. It is handled by handleDeepLinkAndReset()
  /// once the map is ready, because the place page and viewport animations need an initialized map.
  func prepareForColdLaunch(url: URL) {
    isLaunchedByDeepLink = true
    hasPendingColdLaunchDeepLink = true
    self.url = url
  }

  func prepareForColdLaunch(universalLink: URL) {
    guard let url = convertUniversalLink(universalLink) else { return }
    prepareForColdLaunch(url: url)
  }

  func applicationDidOpenUrl(_ url: URL, openInPlace: Bool = false) -> Bool {
    // Files must be imported synchronously: the security-scoped access granted for the URL ends when
    // the scene delegate returns.
    if url.isFileURL {
      return handleFileImport(url: url, openInPlace: openInPlace)
    }

    self.url = url
    // Set before handling: handleDeepLink() opens the screen that reads getInAppFeatureHighlightData().
    isLaunchedByDeepLink = true

    // A link arriving before the map is ready supersedes the previous pending link.
    guard !hasPendingColdLaunchDeepLink else { return true }

    // On the hot start, link can be processed immediately.
    return handleDeepLink(url: url)
  }

  func applicationDidReceiveUniversalLink(_ universalLink: URL) -> Bool {
    guard let url = convertUniversalLink(universalLink) else { return false }
    return applicationDidOpenUrl(url)
  }

  func reset() {
    isLaunchedByDeepLink = false
    hasPendingColdLaunchDeepLink = false
    url = nil
  }

  func getBackUrl() -> String? {
    guard let urlString = url?.absoluteString else { return nil }
    guard let url = URLComponents(string: urlString) else { return nil }
    return (url.queryItems?.first(where: { $0.name == "backurl" })?.value ?? nil)
  }

  func getInAppFeatureHighlightData() -> DeepLinkInAppFeatureHighlightData? {
    guard isLaunchedByDeepLink, let url else { return nil }
    // Highlight the feature once, but keep the URL: goBack() still reads getBackUrl() from it.
    isLaunchedByDeepLink = false
    return DeepLinkInAppFeatureHighlightData(DeepLinkParser.parseAndSetApiURL(url))
  }

  func handleDeepLinkAndReset() -> Bool {
    guard let url else {
      LOG(.error, "handleDeepLink is called with nil URL")
      return false
    }

    let handled = handleDeepLink(url: url)
    reset()
    return handled
  }

  private func handleFileImport(url: URL, openInPlace: Bool) -> Bool {
    LOG(.info, "handleFileImport: \(url), openInPlace: \(openInPlace)")
    guard openInPlace else {
      DeepLinkParser.addBookmarksFile(url, isTemporaryFile: true)
      return true
    }

    let shouldStopAccessing = url.startAccessingSecurityScopedResource()
    defer {
      if shouldStopAccessing {
        url.stopAccessingSecurityScopedResource()
      }
    }

    let fileCoordinator = NSFileCoordinator()
    var error: NSError?
    var copyError: Error?
    fileCoordinator.coordinate(readingItemAt: url, options: [], error: &error) { fileURL in
      do {
        try DeepLinkParser.addBookmarksFile(copyFileToTemporaryDirectory(fileURL), isTemporaryFile: true)
      } catch {
        copyError = error
      }
    }
    if let error {
      LOG(.error, "Failed to read file: \(error)")
    }
    if let copyError {
      LOG(.error, "Failed to copy file for import: \(copyError)")
    }
    return error == nil && copyError == nil
  }

  private func convertUniversalLink(_ universalLink: URL) -> URL? {
    // Convert http(s)://omaps.app/ENCODEDCOORDS/NAME to om://ENCODEDCOORDS/NAME.
    URL(string: universalLink.absoluteString
      .replacingOccurrences(of: "http://omaps.app", with: "om:/")
      .replacingOccurrences(of: "https://omaps.app", with: "om:/"))
  }

  private func copyFileToTemporaryDirectory(_ url: URL) throws -> URL {
    let temporaryDirectory = FileManager.default.temporaryDirectory
      .appendingPathComponent("FileImports/\(UUID().uuidString)", isDirectory: true)
    try FileManager.default.createDirectory(at: temporaryDirectory, withIntermediateDirectories: true)

    let localCopyURL = temporaryDirectory.appendingPathComponent(url.lastPathComponent)
    try FileManager.default.copyItem(at: url, to: localCopyURL)
    return localCopyURL
  }

  private func handleDeepLink(url: URL) -> Bool {
    LOG(.info, "handleDeepLink: \(url)")

    var url = url
    if #available(iOS 18.4, *), let omURL = GeoNavigationToOMURLConverter.convert(url) {
      LOG(.info, "Default navigation app link is converted from \(url) to \(omURL)")
      url = omURL
    }

    // TODO(AB): Rewrite API so iOS and Android will call only one C++ method to clear/set API state.
    // This call is also required for DeepLinkParser.showMap, and it also clears old API points...
    let urlType = DeepLinkParser.parseAndSetApiURL(url)
    LOG(.info, "URL type: \(urlType)")
    switch urlType {
    case .route:
      if let adapter = DeepLinkRouteStrategyAdapter(url) {
        MWMRouter.buildApiRoute(with: adapter.type, start: adapter.p1, finish: adapter.p2)
        MapsAppDelegate.theApp().showMap()
        return true
      }
      return false
    case .map:
      DeepLinkParser.executeMapApiRequest()
      MapsAppDelegate.theApp().showMap()
      return true
    case .search:
      let sd = DeepLinkSearchData()
      let kSearchInViewportZoom: Int32 = 16
      // Set viewport only when cll parameter was provided in url.
      // Equator and Prime Meridian are perfectly valid separately.
      if sd.hasValidCenterLatLon() {
        MapViewController.setViewport(sd.centerLat, lon: sd.centerLon, zoomLevel: kSearchInViewportZoom)
        // Need to update viewport for search API manually because Drape engine
        // will not notify subscribers when search view is shown.
        if !sd.isSearchOnMap {
          sd.onViewportChanged(kSearchInViewportZoom)
        }
      }
      let searchQuery = SearchQuery(sd.query, locale: sd.locale, source: .deeplink)
      if sd.isSearchOnMap {
        MWMMapViewControlsManager.manager()?.search(onMap: searchQuery)
      } else {
        MWMMapViewControlsManager.manager()?.search(searchQuery)
      }
      return true
    case .menu:
      MapsAppDelegate.theApp().mapViewController.openMenu()
      return true
    case .settings:
      MapsAppDelegate.theApp().mapViewController.openSettings()
      return true
    case .crosshair:
      // Not supported on iOS.
      return false
    case .oAuth2:
      MapsAppDelegate.theApp().completeOAuth2Authorization()
      MapsAppDelegate.theApp().mapViewController.closeCurrentView()
      return true
    case .incorrect:
      // Invalid URL or API parameters.
      return false
    @unknown default:
      LOG(.critical, "Unknown URL type: \(urlType)")
      return false
    }
  }
}
