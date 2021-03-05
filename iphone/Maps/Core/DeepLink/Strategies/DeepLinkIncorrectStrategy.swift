class DeepLinkIncorrectStrategy: IDeepLinkHandlerStrategy {
  var deeplinkURL: DeepLinkURL
  var data: IDeepLinkData?

  init(url: DeepLinkURL, data: IDeepLinkData? = nil) {
    deeplinkURL = url
    self.data = data
  }

  func execute() {
    LOG(.warning, "Incorrect parsing result for url: \(deeplinkURL.url)")
  }
}
