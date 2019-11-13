class DeepLinkGeoStrategy: IDeepLinkHandlerStrategy {
  var deeplinkURL: DeepLinkURL

  required init(url: DeepLinkURL) {
    self.deeplinkURL = url
  }

  func execute() {
    if (DeepLinkParser.showMap(for: deeplinkURL.url)) {
      MapsAppDelegate.theApp().showMap()
      Statistics.logEvent(kStatEventName(kStatApplication, kStatImport),
                          withParameters: [kStatValue: deeplinkURL.url.scheme ?? ""])
    }
  }
}
