class DeepLinkRouteStrategy: IDeepLinkHandlerStrategy {
  var deeplinkURL: DeepLinkURL

  init(url: DeepLinkURL) {
    self.deeplinkURL = url
  }

  func execute() {
    if let adapter = DeepLinkRouteStrategyAdapter(deeplinkURL.url) {
      MWMRouter.buildApiRoute(with: adapter.type, start: adapter.p1, finish: adapter.p2)
      MapsAppDelegate.theApp().showMap()
      sendStatisticsOnSuccess(type: kStatRoute)
    } else {
      sendStatisticsOnFail(type: kStatRoute)
    }
  }
}
