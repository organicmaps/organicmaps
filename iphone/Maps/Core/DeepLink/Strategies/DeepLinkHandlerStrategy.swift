protocol IDeepLinkHandlerStrategy {
  var deeplinkURL: DeepLinkURL { get }

  func execute()
}
