class DeepLinkStrategyFactory {
  static func create(url deeplinkURL: DeepLinkURL) -> IDeepLinkHandlerStrategy {
    switch deeplinkURL.url.scheme {
    case "geo", "ge0":
      return DeepLinkGeoStrategy(url: deeplinkURL)
    case "file":
      return DeepLinkFileStrategy(url: deeplinkURL)
    case "mapswithme", "mapsme", "mwm":
      return DeepLinkStrategyFactory.createCommon(url: deeplinkURL)
    default:
      return DeepLinkIncorrectStrategy(url: deeplinkURL)
    }
  }

  private static func createCommon(url deeplinkURL: DeepLinkURL) -> IDeepLinkHandlerStrategy {
    let data = DeepLinkParser.parseAndSetApiURL(deeplinkURL.url)
    guard data.success else {
      return DeepLinkIncorrectStrategy(url: deeplinkURL, data: data)
    }
    switch data.urlType {
    case .incorrect:
      return DeepLinkIncorrectStrategy(url: deeplinkURL, data: data)
    case .route:
      return DeepLinkRouteStrategy(url: deeplinkURL)
    case .map:
      return DeepLinkMapStrategy(url: deeplinkURL)
    case .search:
      guard let searchData = data as? DeepLinkSearchData else {
        return DeepLinkIncorrectStrategy(url: deeplinkURL)
      }
      return DeepLinkSearchStrategy(url: deeplinkURL, data: searchData)
    case .catalogue:
      return DeepLinkCatalogueStrategy(url: deeplinkURL)
    case .cataloguePath:
      return DeepLinkCataloguePathStrategy(url: deeplinkURL)
    case .subscription:
      guard let subscriptionData = data as? DeepLinkSubscriptionData else {
        return DeepLinkIncorrectStrategy(url: deeplinkURL)
      }
      return DeepLinkSubscriptionStrategy(url: deeplinkURL, data: subscriptionData)
    case .lead:
      return DeepLinkLeadStrategy(url: deeplinkURL)
    }
  }
}
