class DeepLinkIncorrectStrategy: IDeepLinkHandlerStrategy {
  var deeplinkURL: DeepLinkURL
  var data: IDeepLinkData?

  init(url: DeepLinkURL, data: IDeepLinkData? = nil) {
    deeplinkURL = url
    self.data = data
  }

  func execute() {
    LOG(.warning, "Incorrect parsing result for url: \(deeplinkURL.url)")
    sendStatisticsOnFail(type: data?.urlType.getStatName() ?? kStatUnknown)
  }
}

private extension DeeplinkUrlType {
  func getStatName() -> String {
    switch self {
    case .catalogue:
      return kStatCatalogue
    case .cataloguePath:
      return kStatCataloguePath
    case .incorrect:
      return kStatUnknown
    case .lead:
      return kStatLead
    case .map:
      return kStatMap
    case .route:
      return kStatRoute
    case .search:
      return kStatSearch
    case .subscription:
      return kStatSubscription
    @unknown default:
      fatalError()
    }
  }
}
