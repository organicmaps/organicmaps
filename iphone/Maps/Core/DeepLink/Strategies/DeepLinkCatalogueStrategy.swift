class DeepLinkCatalogueStrategy: IDeepLinkHandlerStrategy {
  var deeplinkURL: DeepLinkURL

  init(url: DeepLinkURL) {
    self.deeplinkURL = url
  }

  func execute() {
    MapViewController.shared()?.openCatalogDeeplink(deeplinkURL.url, animated: false, utm: .none);
    sendStatisticsOnSuccess(type: kStatCatalogue)
  }
}
