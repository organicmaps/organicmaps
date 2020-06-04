@objc enum DeepLinkProvider: Int {
  case native
  case appsflyer

  var statName: String {
    switch self {
    case .native:
      return kStatNative
    case .appsflyer:
      return kStatAppsflyer
    }
  }
}

class DeepLinkURL {
  let url: URL
  let provider: DeepLinkProvider

  init(url: URL, provider: DeepLinkProvider = .native) {
    self.url = url
    self.provider = provider
  }
}

@objc @objcMembers class DeepLinkHandler: NSObject {
  static let shared = DeepLinkHandler()

  private(set) var isLaunchedByDeeplink = false
  private(set) var deeplinkURL: DeepLinkURL?

  var needExtraWelcomeScreen: Bool {
    guard let host = deeplinkURL?.url.host else { return false }
    return host == "catalogue" || host == "guides_page"
  }

  private var canHandleLink = false

  private override init() {
    super.init()
  }

  func applicationDidFinishLaunching(_ options: [UIApplication.LaunchOptionsKey : Any]? = nil) {
    if let userActivityOptions = options?[.userActivityDictionary] as? [UIApplication.LaunchOptionsKey : Any],
      let userActivityType = userActivityOptions[.userActivityType] as? String,
      userActivityType == NSUserActivityTypeBrowsingWeb {
      isLaunchedByDeeplink = true
    }

    if let launchDeeplink = options?[UIApplication.LaunchOptionsKey.url] as? URL {
      isLaunchedByDeeplink = true
      deeplinkURL = DeepLinkURL(url: launchDeeplink)
    }
  }

  func applicationDidOpenUrl(_ url: URL) -> Bool {
    deeplinkURL = DeepLinkURL(url: url)
    if canHandleLink {
      handleInternal()
    }
    return true
  }

  private func setUniversalLink(_ url: URL, provider: DeepLinkProvider) -> Bool {
    let dlUrl = convertUniversalLink(url)
    guard deeplinkURL == nil else { return false }
    deeplinkURL = DeepLinkURL(url: dlUrl)
    return true
  }

  func applicationDidReceiveUniversalLink(_ url: URL) -> Bool {
    return applicationDidReceiveUniversalLink(url, provider: .native)
  }

  func applicationDidReceiveUniversalLink(_ url: URL, provider: DeepLinkProvider) -> Bool {
    var result = false
    if let host = url.host, host == "mapsme.onelink.me" {
      URLComponents(url: url, resolvingAgainstBaseURL: false)?.queryItems?.forEach {
        if $0.name == "af_dp" {
          guard let value = $0.value, let dl = URL(string: value) else { return }
          result = setUniversalLink(dl, provider: provider)
        }
      }
    } else {
      result = setUniversalLink(url, provider: provider)
    }
    if canHandleLink {
      handleInternal()
    }
    return result
  }

  func handleDeeplink() {
    canHandleLink = true
    if deeplinkURL != nil {
      handleInternal()
    }
  }

  func handleDeeplink(_ url: URL) {
    deeplinkURL = DeepLinkURL(url: url)
    handleDeeplink()
  }

  func reset() {
    isLaunchedByDeeplink = false
    deeplinkURL = nil
  }

  func getBackUrl() -> String {
    guard let urlString = deeplinkURL?.url.absoluteString else { return "" }
    guard let url = URLComponents(string: urlString) else { return "" }
    return (url.queryItems?.first(where: { $0.name == "backurl" })?.value ?? "")
  }

  private func convertUniversalLink(_ universalLink: URL) -> URL {
    let convertedLink = String(format: "mapsme:/%@?%@", universalLink.path, universalLink.query ?? "")
    return URL(string: convertedLink)!
  }

  private func handleInternal() {
    guard let url = deeplinkURL else {
      assertionFailure()
      return
    }
    LOG(.info, "Handle deeplink: \(url.url)")
    let deeplinkHandlerStrategy = DeepLinkStrategyFactory.create(url: url)
    deeplinkHandlerStrategy.execute()
  }
}
