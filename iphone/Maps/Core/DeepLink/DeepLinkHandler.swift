@objc @objcMembers class DeepLinkHandler: NSObject {
  static let shared = DeepLinkHandler()

  private(set) var isLaunchedByDeeplink = false
  private(set) var isLaunchedByUniversalLink = false
  private(set) var url: URL?

  override private init() {
    super.init()
  }

  func applicationDidFinishLaunching(_ options: [UIApplication.LaunchOptionsKey: Any]? = nil) {
    if let launchDeeplink = options?[UIApplication.LaunchOptionsKey.url] as? URL {
      isLaunchedByDeeplink = true
      url = launchDeeplink
    }
  }

  func applicationDidOpenUrl(_ url: URL, options: [UIApplication.OpenURLOptionsKey: Any] = [:]) -> Bool {
    // File reading should be processed synchronously to avoid permission issues (the Files app will close the file for reading when the application:openURL:options returns).
    if url.isFileURL {
      // UIApplication.openURL options are deprecated for scene-based apps. When scenes are adopted,
      // handle UIApplication.OpenURLOptionsKey.openInPlace via the scene URL context instead.
      // https://developer.apple.com/documentation/uikit/uiapplication/openurloptionskey/openinplace?language=objc
      let openInPlace = options[.openInPlace] as? Bool ?? false
      return handleFileImport(url: url, openInPlace: openInPlace)
    }

    // On the cold start, isLaunchedByDeeplink is set and handleDeepLink() call is delayed
    // until the map view will be fully initialized.
    guard !isLaunchedByDeeplink else { return true }

    // On the hot start, link can be processed immediately.
    self.url = url
    return handleDeepLink(url: url)
  }

  func applicationDidReceiveUniversalLink(_ universalLink: URL) -> Bool {
    // Convert http(s)://omaps.app/ENCODEDCOORDS/NAME to om://ENCODEDCOORDS/NAME
    url = URL(string: universalLink.absoluteString
      .replacingOccurrences(of: "http://omaps.app", with: "om:/")
      .replacingOccurrences(of: "https://omaps.app", with: "om:/"))
    isLaunchedByUniversalLink = true
    return handleDeepLink(url: url!)
  }

  func reset() {
    isLaunchedByDeeplink = false
    isLaunchedByUniversalLink = false
    url = nil
  }

  func getBackUrl() -> String? {
    guard let urlString = url?.absoluteString else { return nil }
    guard let url = URLComponents(string: urlString) else { return nil }
    if let backUrl = url.queryItems?.first(where: { $0.name == "backurl" })?.value {
      return backUrl
    }

    guard isRouteApiV2(url) else { return nil }
    return url.queryItems?.first(where: { $0.name == "callback" })?.value
  }

  private func isRouteApiV2(_ url: URLComponents) -> Bool {
    switch (url.host, url.path) {
    case ("v2", "/dir"), ("v2", "/nav"),
         ("omaps.app", "/v2/dir"), ("omaps.app", "/v2/nav"):
      return true
    default:
      return false
    }
  }

  func getInAppFeatureHighlightData() -> DeepLinkInAppFeatureHighlightData? {
    guard isLaunchedByUniversalLink || isLaunchedByDeeplink, let url else { return nil }
    reset()
    return DeepLinkInAppFeatureHighlightData(DeepLinkParser.parseAndSetApiURL(url))
  }

  func handleDeepLinkAndReset() -> Bool {
    if let url {
      let result = handleDeepLink(url: url)
      reset()
      return result
    }
    LOG(.error, "handleDeepLink is called with nil URL")
    return false
  }

  private func handleFileImport(url: URL, openInPlace: Bool) -> Bool {
    LOG(.info, "handleFileImport: \(url), openInPlace: \(openInPlace)")
    guard openInPlace else {
      DeepLinkParser.addBookmarksFile(url, isTemporaryFile: true)
      reset()
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
    reset()
    return error == nil && copyError == nil
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
        MWMRouter.buildApiRoute(with: adapter.type,
                                start: adapter.start,
                                intermediatePoints: adapter.intermediatePoints,
                                finish: adapter.finish,
                                optimizeRoutePoints: adapter.optimizeRoutePoints,
                                startRouteNavigation: adapter.startRouteNavigation,
                                startDirection: adapter.startDirection)
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
