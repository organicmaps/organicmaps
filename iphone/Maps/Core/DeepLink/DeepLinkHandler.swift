@objc @objcMembers class DeepLinkHandler: NSObject {
  static let shared = DeepLinkHandler()

  private(set) var isLaunchedByDeeplink = false
  private(set) var url: URL?

  private override init() {
    super.init()
  }

  func applicationDidFinishLaunching(_ options: [UIApplication.LaunchOptionsKey : Any]? = nil) {
    if let launchDeeplink = options?[UIApplication.LaunchOptionsKey.url] as? URL {
      isLaunchedByDeeplink = true
      url = launchDeeplink
    }
  }

  func applicationDidOpenUrl(_ url: URL) -> Bool {
    // On the cold start, isLaunchedByDeeplink is set and handleDeepLink() call is delayed
    // until the map view will be fully initialized.
    guard !isLaunchedByDeeplink else { return true }

    // On the hot start, link can be processed immediately.
    self.url = url
    return handleDeepLink(url: url)
  }

  func applicationDidReceiveUniversalLink(_ universalLink: URL) -> Bool {
    // Convert http(s)://omaps.app/ENCODEDCOORDS/NAME to om://ENCODEDCOORDS/NAME
    self.url = URL(string: universalLink.absoluteString
                    .replacingOccurrences(of: "http://omaps.app", with: "om:/")
                    .replacingOccurrences(of: "https://omaps.app", with: "om:/"))
    return handleDeepLink(url: self.url!)
  }

  func reset() {
    isLaunchedByDeeplink = false
    url = nil
  }

  func getBackUrl() -> String? {
    guard let urlString = url?.absoluteString else { return nil }
    guard let url = URLComponents(string: urlString) else { return nil }
    return (url.queryItems?.first(where: { $0.name == "backurl" })?.value ?? nil)
  }

  func handleDeepLinkAndReset() -> Bool {
    if let url = self.url {
      let result = handleDeepLink(url: url)
      reset()
      return result
    }
    LOG(.error, "handleDeepLink is called with nil URL")
    return false
  }
  
  private func handleDeepLink(url: URL) -> Bool {
    LOG(.info, "handleDeepLink: \(url)")

    if url.scheme == "file" {
      // Import bookmarks.
      DeepLinkParser.addBookmarksFile(url)
      return true  // Async parsing can fail later, but here we always return true.
    }

    // TODO(AB): Rewrite API so iOS and Android will call only one C++ method to clear/set API state.
    // This call is also required for DeepLinkParser.showMap, and it also clears old API points...
    let urlType = DeepLinkParser.parseAndSetApiURL(url)
    switch urlType {

    case .route:
      if let adapter = DeepLinkRouteStrategyAdapter(url) {
        MWMRouter.buildApiRoute(with: adapter.type, start: adapter.p1, finish: adapter.p2)
        MapsAppDelegate.theApp().showMap()
        return true
      }
      return false;

    case .map:
      DeepLinkParser.executeMapApiRequest()
      MapsAppDelegate.theApp().showMap()
      return true

    case .search:
      let sd = DeepLinkSearchData();
      let kSearchInViewportZoom: Int32 = 16;
      // Set viewport only when cll parameter was provided in url.
      // Equator and Prime Meridian are perfectly valid separately.
      if (sd.hasValidCenterLatLon()) {
        MapViewController.setViewport(sd.centerLat, lon: sd.centerLon, zoomLevel: kSearchInViewportZoom)
        // Need to update viewport for search API manually because Drape engine
        // will not notify subscribers when search view is shown.
        if (!sd.isSearchOnMap) {
          sd.onViewportChanged(kSearchInViewportZoom)
        }
      }
      if (sd.isSearchOnMap) {
        MWMMapViewControlsManager.manager()?.searchText(onMap: sd.query, forInputLocale: sd.locale)
      } else {
        MWMMapViewControlsManager.manager()?.searchText(sd.query, forInputLocale: sd.locale)
      }
      return true

    case .crosshair:
      // Not supported on iOS.
      return false;

    case .incorrect:
      // Invalid URL or API parameters.
      return false;
    }
  }
}
