class DeepLinkIncorrectStrategy: IDeepLinkHandlerStrategy {
  var deeplinkURL: DeepLinkURL

  init(url: DeepLinkURL) {
    self.deeplinkURL = url
  }

  func execute() {
    LOG(.warning, "Incorrect parsing result for url: \(deeplinkURL.url)");
    sendStatisticsOnFail(type: kStatUnknown)
  }
}
