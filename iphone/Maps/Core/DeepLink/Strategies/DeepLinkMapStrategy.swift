class DeepLinkMapStrategy: IDeepLinkHandlerStrategy {
  var deeplinkURL: DeepLinkURL

  init(url: DeepLinkURL) {
    self.deeplinkURL = url
  }

  func execute() {
    if (DeepLinkParser.showMap(for: deeplinkURL.url)) {
      MapsAppDelegate.theApp().showMap()
      sendStatisticsOnSuccess(type: kStatMap)
    } else {
      sendStatisticsOnFail(type: kStatMap)
    }
  }
}
