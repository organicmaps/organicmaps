class DeepLinkLeadStrategy: IDeepLinkHandlerStrategy {
  var deeplinkURL: DeepLinkURL

  init(url: DeepLinkURL) {
    self.deeplinkURL = url
  }

  func execute() {
    sendStatisticsOnSuccess(type: kStatLead)
  }
}
