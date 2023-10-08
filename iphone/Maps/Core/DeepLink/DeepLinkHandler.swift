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

  // Hacky check if it's ge0 style link.
  private func isGe0Link(url: URL) -> Bool {
    if url.query != nil {
      return false;
    }
    switch (url.path) {
    case "map": fallthrough
    case "search": fallthrough
    case "route": fallthrough
    case "crosshair": return false

    default: return true
    }
  }
  
  private func handleDeepLink(url: URL) -> Bool {
    LOG(.info, "handleDeepLink: \(url)")

    // TODO(AB): Rewrite API so iOS and Android will call only one C++ method to clear/set API state.
    // This call is also required for DeepLinkParser.showMap, and it also clears old API points...
    let dlData = DeepLinkParser.parseAndSetApiURL(url)

    switch url.scheme {
      // Process old Maps.Me url schemes.
      case "geo", "ge0":
        if DeepLinkParser.showMap(for: url) {
          MapsAppDelegate.theApp().showMap()
          return true
        }
      // Import bookmarks.
      case "file":
        DeepLinkParser.addBookmarksFile(url)
        return true  // We don't really know if async parsing was successful.
      case  "om":
        // TODO(AB): DeepLinkParser.showMap uses points that are reset in DeepLinkParser.parseAndSetApiURL.
        // Need a complete rewrite if API parsing and processing logic, now use a crude check for omaps.app style urls.
        // It could be either a renamed ge0 link...
        if isGe0Link(url: url) {
          if DeepLinkParser.showMap(for: url) {
            MapsAppDelegate.theApp().showMap()
            return true
          }
          return false
        }
        // ...or an API scheme.
        fallthrough
      // API scheme.
      case "mapswithme", "mapsme", "mwm":
        guard dlData.success else { return false }

        switch dlData.urlType {

          case .route:
            if let adapter = DeepLinkRouteStrategyAdapter(url) {
              MWMRouter.buildApiRoute(with: adapter.type, start: adapter.p1, finish: adapter.p2)
              MapsAppDelegate.theApp().showMap()
              return true
            }

          case .map:
            if DeepLinkParser.showMap(for: url) {
              MapsAppDelegate.theApp().showMap()
              return true
            }

          case .search:
            if let sd = dlData as? DeepLinkSearchData {
              let kSearchInViewportZoom: Int32 = 16;
              // Set viewport only when cll parameter was provided in url.
              // Equator and Prime Meridian are perfectly valid separately.
              if (sd.centerLat != 0.0 || sd.centerLon != 0.0) {
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
            }

          // Invalid API parameters.
          default: break
        }
      // Not supported url schemes.
      default: break
    }
    return false
  }
}
