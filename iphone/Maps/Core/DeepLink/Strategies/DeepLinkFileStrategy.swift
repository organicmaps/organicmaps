import Foundation

class DeepLinkFileStrategy: IDeepLinkHandlerStrategy {
  var deeplinkURL: DeepLinkURL

  init(url: DeepLinkURL) {
    self.deeplinkURL = url
  }

  func execute() {
    DeepLinkParser.addBookmarksFile(deeplinkURL.url)
  }
}
